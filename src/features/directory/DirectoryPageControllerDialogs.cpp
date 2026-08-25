#include "DirectoryPageController.h"

#include "DirectoryGateway.h"
#include "DirectoryLoadCoordinator.h"
#include "DirectoryPageDialog.h"
#include "DirectorySelectionPolicy.h"
#include "DirectorySelectionDialog.h"
#include "DirectoryTabPage.h"
#include "DirectoryTabPresentation.h"
#include "DirectoryModule.h"
#include "FileBrowser.h"
#include "NodeGateway.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSpacerItem>
#include <QTabWidget>
#include <QThreadPool>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <functional>

namespace {
QTreeWidgetItem *findDirectoryTreeItem(QTreeWidget *tree, const QString &path)
{
    if (!tree || path.isEmpty()) {
        return nullptr;
    }

    std::function<QTreeWidgetItem *(QTreeWidgetItem *)> findInItem =
        [&findInItem, &path](QTreeWidgetItem *item) -> QTreeWidgetItem * {
        if (!item) {
            return nullptr;
        }
        if (item->data(0, Qt::UserRole).toString() == path) {
            return item;
        }
        for (int i = 0; i < item->childCount(); ++i) {
            if (QTreeWidgetItem *match = findInItem(item->child(i))) {
                return match;
            }
        }
        return nullptr;
    };

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (QTreeWidgetItem *match = findInItem(tree->topLevelItem(i))) {
            return match;
        }
    }
    return nullptr;
}
}
/**
 * @brief 打开目录选择对话框，并在确认后回收节点和路径结果。
 * @param initialNodeIndex 初始节点下拉索引。
 * @param initialPath 对话框初始目录路径。
 * @param result 返回的目录选择结果。
 * @param allowNodeSelection 兼容参数；当前实现不主动禁用节点选择器。
 * @return 用户是否确认并得到有效目录结果。
 */
bool DirectoryPageController::openDirectorySelectionDialog(int initialNodeIndex,
                                                           const QString &initialPath,
                                                           DirectorySelectionResult &result,
                                                           bool allowNodeSelection)
{
    Q_UNUSED(allowNodeSelection);
    DirectorySelectionDialog dialog(m_messageParent);
    DirectorySelectionDialog *dialogPtr = &dialog;
    QComboBox *dirNodeComboBox = dialog.nodeComboBox();
    QLineEdit *searchEdit = dialog.searchEdit();
    QPushButton *searchButton = dialog.searchButton();
    QTreeWidget *dirTree = dialog.directoryTree();
    QPushButton *dirDialogCancelButton = dialog.cancelButton();
    QPushButton *dirDialogOkButton = dialog.okButton();

    populateDownloadNodeCombo(dirNodeComboBox);
    dirNodeComboBox->setCurrentIndex(initialNodeIndex);
    m_fileBrowser->configureDirectorySelectionTree(dirTree);

    QPointer<DirectorySelectionDialog> dialogGuard(&dialog);
    QPointer<DirectoryPageController> controllerGuard(this);
    QPointer<DirectoryGateway> directoryGatewayGuard(m_directoryGateway);
    QPointer<FileBrowser> fileBrowserGuard(m_fileBrowser);
    DirectoryLoadCoordinator loadCoordinator(
        [directoryGatewayGuard](const QString &nodeId, const QString &path) {
            return directoryGatewayGuard
                ? directoryGatewayGuard->fileInfoList(nodeId, path)
                : QList<NetworkFileInfo>();
        });

    auto loadDirectory = [&](const QString &nodeId, const QString &path, QTreeWidgetItem *parentItem = nullptr) {
        if (nodeId.isEmpty()) {
            return;
        }

        const QString parentPath = parentItem
            ? parentItem->data(0, Qt::UserRole).toString()
            : QString();

        // 每次重新切节点或切目录时，都让旧的异步响应自然失效。
        if (!parentItem) {
            dirTree->clear();
            QTreeWidgetItem *loadingItem = new QTreeWidgetItem(dirTree);
            loadingItem->setText(0, QStringLiteral("正在加载目录..."));
            loadingItem->setFlags(loadingItem->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
            loadingItem->setForeground(0, QColor(100, 180, 255));
            dirTree->addTopLevelItem(loadingItem);
        }

        loadCoordinator.request(nodeId, path,
                                [controllerGuard, fileBrowserGuard, dialogGuard,
                                 dirNodeComboBox, dirTree, parentPath](quint64,
                                                                       const QString &requestedNodeId,
                                                                       const QString &requestedPath,
                                                                       const QList<NetworkFileInfo> &fileList) {
            if (!controllerGuard || !fileBrowserGuard || !dialogGuard
                || dirNodeComboBox->currentData().toString() != requestedNodeId) {
                return;
            }

            // 只用稳定的路径重新定位树项，不在异步回调中解引用旧 QTreeWidgetItem*。
            if (parentPath.isEmpty()) {
                fileBrowserGuard->populateDirectorySelectionTree(dirTree, fileList);
                return;
            }

            QTreeWidgetItem *currentParent = findDirectoryTreeItem(dirTree, parentPath);
            if (!currentParent || currentParent->data(0, Qt::UserRole).toString() != parentPath) {
                return;
            }
            fileBrowserGuard->populateDirectorySelectionTree(dirTree, fileList, currentParent);
            Q_UNUSED(requestedPath);
        });
    };

    loadDirectory(dirNodeComboBox->currentData().toString(), initialPath.isEmpty() ? QStringLiteral("/") : initialPath);

    connect(dirNodeComboBox, &QComboBox::currentIndexChanged, dialogPtr, [=]() {
        const QString newNodeId = dirNodeComboBox->currentData().toString();
        if (!newNodeId.isEmpty()) {
            loadDirectory(newNodeId, QStringLiteral("/"));
        }
    });

    connect(dirTree, &QTreeWidget::itemExpanded, dialogPtr, [=](QTreeWidgetItem *item) {
        if (!item || item->childCount() != 1) {
            return;
        }
        QTreeWidgetItem *child = item->child(0);
        if (!child || child->text(0) != QStringLiteral("正在加载...")) {
            return;
        }

        delete item->takeChild(0);
        const QString dirPath = item->data(0, Qt::UserRole).toString();
        const QString currentNodeId = dirNodeComboBox->currentData().toString();
        if (!currentNodeId.isEmpty()) {
            loadDirectory(currentNodeId, dirPath, item);
        }
    });

    connect(dirTree, &QTreeWidget::itemDoubleClicked, dialogPtr, [this, dialogPtr, dirNodeComboBox, &result](QTreeWidgetItem *item, int) {
        if (!item) {
            return;
        }
        const QString dirPath = item->data(0, Qt::UserRole).toString();
        if (dirPath.isEmpty()) {
            return;
        }
        applyDirectorySelectionResult(dirNodeComboBox, dirPath, result);
        dialogPtr->accept();
    });

    connect(searchButton, &QPushButton::clicked, dialogPtr, [=]() {
        const QString searchText = searchEdit->text();
        if (searchText.trimmed().isEmpty()) {
            return;
        }
        const int foundCount = m_directoryModule->searchDirectoryTree(dirTree, searchText);
        if (foundCount <= 0) {
            QMessageBox::information(dialogPtr, QStringLiteral("搜索结果"), QStringLiteral("没有找到匹配的目录"));
        }
    });
    connect(searchEdit, &QLineEdit::returnPressed, searchButton, &QPushButton::click);

    connect(dirDialogCancelButton, &QPushButton::clicked, dialogPtr, &QDialog::reject);
    connect(dirDialogOkButton, &QPushButton::clicked, dialogPtr, [this, dialogPtr, dirTree, dirNodeComboBox, &result]() {
        QTreeWidgetItem *selectedItem = dirTree->currentItem();
        if (!selectedItem) {
            dialogPtr->accept();
            return;
        }
        const QString dirPath = selectedItem->data(0, Qt::UserRole).toString();
        if (!dirPath.isEmpty()) {
            applyDirectorySelectionResult(dirNodeComboBox, dirPath, result);
        }
        dialogPtr->accept();
    });

    const bool accepted = (dialog.exec() == QDialog::Accepted);
    return DirectorySelectionPolicy::isValidResult(accepted, result.nodeIndex, result.path);
}

/**
 * @brief 打开新建目录页对话框，并在确认后创建或复用目录页签。
 * @param initialNodeIndex 初始节点下拉索引。
 */
void DirectoryPageController::openDirectoryPageDialog(int initialNodeIndex)
{
    DirectoryPageDialog dialog(m_messageParent);
    QPointer<DirectoryPageDialog> dialogGuard(&dialog);
    QPointer<DirectoryPageController> controllerGuard(this);
    QPointer<DirectoryGateway> directoryGatewayGuard(m_directoryGateway);
    QPointer<FileBrowser> fileBrowserGuard(m_fileBrowser);
    DirectoryLoadCoordinator loadCoordinator(
        [directoryGatewayGuard](const QString &nodeId, const QString &path) {
            return directoryGatewayGuard
                ? directoryGatewayGuard->fileInfoList(nodeId, path)
                : QList<NetworkFileInfo>();
        });
    DirectoryPageDialog *dialogPtr = dialogGuard.data();
    QComboBox *nodeComboBox = dialog.nodeComboBox();
    QLineEdit *pathEdit = dialog.pathEdit();
    QPushButton *browseButton = dialog.browseButton();
    QPushButton *refreshButton = dialog.refreshButton();
    QTreeWidget *folderTree = dialog.folderTree();
    QPushButton *cancelButton = dialog.cancelButton();
    QPushButton *okButton = dialog.okButton();

    populateDownloadNodeCombo(nodeComboBox);
    nodeComboBox->setCurrentIndex(initialNodeIndex);
    m_fileBrowser->configureFileTree(folderTree);

    auto loadDirList = [this, controllerGuard, fileBrowserGuard,
                        dialogGuard, nodeComboBox, folderTree, pathEdit,
                        &loadCoordinator]() {
        if (!controllerGuard || !dialogGuard) {
            return;
        }

        QString nodeId;
        if (!ensureDialogDownloadNodeSelected(dialogGuard.data(), nodeComboBox, nodeId)) {
            folderTree->clear();
            return;
        }

        const QString requestedPath = pathEdit->text();
        loadCoordinator.request(nodeId, requestedPath,
                                [controllerGuard, fileBrowserGuard, dialogGuard,
                                 nodeComboBox, pathEdit, folderTree,
                                 nodeId, requestedPath](quint64,
                                                        const QString &,
                                                        const QString &,
                                                        const QList<NetworkFileInfo> &fileList) {
            if (!controllerGuard || !fileBrowserGuard || !dialogGuard
                || nodeComboBox->currentData().toString() != nodeId
                || pathEdit->text() != requestedPath) {
                return;
            }
            fileBrowserGuard->populateDirectoryListingTree(folderTree, fileList);
        });
    };

    connect(browseButton, &QPushButton::clicked, this, [=]() {
        DirectorySelectionResult selection;
        if (!openDirectorySelectionDialog(nodeComboBox->currentIndex(), pathEdit->text(), selection, false)) {
            return;
        }
        pathEdit->setText(selection.path);
        loadDirList();
    });
    connect(refreshButton, &QPushButton::clicked, loadDirList);
    connect(folderTree, &QTreeWidget::itemDoubleClicked, [=](QTreeWidgetItem *item, int) {
        if (item && item->data(1, Qt::UserRole).toBool()) {
            pathEdit->setText(item->data(0, Qt::UserRole).toString());
            loadDirList();
        }
    });
    connect(cancelButton, &QPushButton::clicked, dialogPtr, &QDialog::reject);
    connect(okButton, &QPushButton::clicked, this, [this, dialogPtr, nodeComboBox, pathEdit]() {
        QString nodeId;
        QString nodeName;
        if (!ensureDialogDownloadNodeSelected(dialogPtr, nodeComboBox, nodeId, &nodeName)) {
            return;
        }

        const QString dirPath = pathEdit->text();
        const int existingTabIndex = findExistingNodeTabIndex(nodeId, dirPath);
        // 同一节点同一路径只保留一个页签，避免目录页无限重复。
        if (existingTabIndex >= 0 && activateExistingNodeTab(existingTabIndex, nodeId, dirPath)) {
            dialogPtr->accept();
            return;
        }

        createNodeDirectoryTab(nodeId, nodeName, dirPath);
        dialogPtr->accept();
    });

    loadDirList();
    dialog.exec();
}
