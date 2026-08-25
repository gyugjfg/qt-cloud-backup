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


DirectoryPageController::DirectoryPageController(QTabWidget *nodeTabWidget,
                                                 QComboBox *downloadNodeComboBox,
                                                 FileBrowser *fileBrowser,
                                                 NodeGateway *nodeGateway,
                                                 DirectoryGateway *directoryGateway,
                                                 DirectoryModule *directoryModule,
                                                 QWidget *messageParent,
                                                 QObject *parent)
    : QObject(parent)
    , m_nodeTabWidget(nodeTabWidget)
    , m_downloadNodeComboBox(downloadNodeComboBox)
    , m_fileBrowser(fileBrowser)
    , m_nodeGateway(nodeGateway)
    , m_directoryGateway(directoryGateway)
    , m_directoryModule(directoryModule)
    , m_messageParent(messageParent)
{
    // 目录相关的默认页、动态页签和选择弹窗都从这里统一收口。
}
void DirectoryPageController::populateDownloadNodeCombo(QComboBox *comboBox) const
{
    if (!comboBox || !m_downloadNodeComboBox) {
        return;
    }

    comboBox->clear();
    for (int i = 0; i < m_downloadNodeComboBox->count(); ++i) {
        comboBox->addItem(m_downloadNodeComboBox->itemText(i),
                          m_downloadNodeComboBox->itemData(i));
    }
}

bool DirectoryPageController::ensureDialogDownloadNodeSelected(QWidget *dialogParent,
                                                               QComboBox *nodeComboBox,
                                                               QString &nodeId,
                                                               QString *nodeName) const
{
    if (!nodeComboBox || nodeComboBox->currentIndex() <= 0) {
        QMessageBox::warning(dialogParent, QStringLiteral("警告"), QStringLiteral("请先选择一个节点"));
        return false;
    }

    nodeId = nodeComboBox->currentData().toString();
    if (nodeId.isEmpty()) {
        QMessageBox::warning(dialogParent, QStringLiteral("警告"), QStringLiteral("请先选择一个节点"));
        return false;
    }

    if (nodeName) {
        *nodeName = nodeComboBox->currentText();
    }
    return true;
}

void DirectoryPageController::applyDirectorySelectionResult(QComboBox *nodeComboBox,
                                                            const QString &path,
                                                            DirectorySelectionResult &result) const
{
    if (!nodeComboBox || path.isEmpty()) {
        return;
    }

    result.nodeIndex = nodeComboBox->currentIndex();
    result.nodeId = nodeComboBox->currentData().toString();
    result.path = path;
}

int DirectoryPageController::findExistingNodeTabIndex(const QString &nodeId, const QString &path) const
{
    if (!m_nodeTabWidget) {
        return -1;
    }

    for (int i = 0; i < m_nodeTabWidget->count(); ++i) {
        QWidget *tabWidget = m_nodeTabWidget->widget(i);
        if (!tabWidget) {
            continue;
        }
        if (tabWidget->property("nodeId").toString() == nodeId
            && tabWidget->property("dirPath").toString() == path) {
            return i;
        }
    }
    return -1;
}

bool DirectoryPageController::activateExistingNodeTab(int tabIndex, const QString &nodeId, const QString &path)
{
    if (!m_nodeTabWidget || tabIndex < 0 || tabIndex >= m_nodeTabWidget->count()) {
        return false;
    }

    m_nodeTabWidget->setCurrentIndex(tabIndex);
    QWidget *currentTab = m_nodeTabWidget->currentWidget();
    if (!currentTab) {
        return false;
    }

    const QString nodeName = DirectoryTabPresentation::nodeName(m_nodeTabWidget->tabText(tabIndex));
    applyNodeTabPathState(currentTab, nodeId, nodeName, path);
    return true;
}

QString DirectoryPageController::directoryTabNodeId(QWidget *tab, const QString &fallbackNodeId) const
{
    return m_directoryModule
        ? m_directoryModule->currentTabNodeId(tab, fallbackNodeId)
        : fallbackNodeId;
}

QString DirectoryPageController::directoryTabNodeName(QWidget *tab, const QString &fallbackNodeName) const
{
    return m_directoryModule && m_nodeTabWidget
        ? m_directoryModule->currentTabNodeName(tab, m_nodeTabWidget, fallbackNodeName)
        : fallbackNodeName;
}

void DirectoryPageController::updateDirectoryTabNodeState(QWidget *tab,
                                                          const QString &nodeId,
                                                          const QString &nodeName)
{
    if (!tab || !m_nodeGateway) {
        return;
    }

    tab->setProperty("nodeId", nodeId);

    if (QLabel *nodeNameLabel = tab->findChild<QLabel*>("nodeNameLabel")) {
        nodeNameLabel->setText(nodeName);
    }

    if (QLabel *nodeStatusLabel = tab->findChild<QLabel*>("nodeStatusLabel")) {
        const NodeGateway::NodeInfo nodeInfo = m_nodeGateway->nodeInfo(nodeId);
        const bool nodeOnline = (nodeInfo.status == 1);
        nodeStatusLabel->setText(nodeOnline ? QStringLiteral("在线") : QStringLiteral("离线"));
        nodeStatusLabel->setStyleSheet(nodeOnline
            ? "color: #07856F; font-size: 13px; font-weight: 600;"
            : "color: #C73B52; font-size: 13px; font-weight: 600;");
    }
}
// 刷新目录页签的在线状态。
void DirectoryPageController::refreshDirectoryTabOnlineState(QWidget *tab, const QString &nodeId) const
{
    if (!tab || !m_nodeGateway) {
        return;
    }

    // 在线状态异步回刷时再校验一次当前页签归属，避免旧请求覆盖新节点状态。
    QPointer<QWidget> tabGuard(tab);
    NodeGateway *nodeGateway = m_nodeGateway;
    QThreadPool::globalInstance()->start([tabGuard, nodeId, nodeGateway]() {
        if (!tabGuard || !nodeGateway) {
            return;
        }

        const bool latestOnline = nodeGateway->checkNodeStatus(nodeId);
        QMetaObject::invokeMethod(tabGuard, [tabGuard, nodeId, latestOnline]() {
            if (!tabGuard) {
                return;
            }
            if (tabGuard->property("nodeId").toString() != nodeId) {
                return;
            }

            if (QLabel *nodeStatusLabel = tabGuard->findChild<QLabel*>("nodeStatusLabel")) {
                nodeStatusLabel->setText(latestOnline ? QStringLiteral("在线") : QStringLiteral("离线"));
                nodeStatusLabel->setStyleSheet(latestOnline
                    ? "color: #07856F; font-size: 13px; font-weight: 600;"
                    : "color: #C73B52; font-size: 13px; font-weight: 600;");
            }
        }, Qt::QueuedConnection);
    });
}

/**
 * @brief 把当前目录页切换到指定节点和路径，并同步节点状态与文件树。
 * @param tab 目标页签。
 * @param nodeId 目标节点 ID。
 * @param nodeName 目标节点显示名。
 * @param path 目标路径。
 */
void DirectoryPageController::navigateDirectoryTab(QWidget *tab,
                                                   const QString &nodeId,
                                                   const QString &nodeName,
                                                   const QString &path)
{
    if (!tab || !m_fileBrowser) {
        return;
    }

    m_fileBrowser->setCurrentPath(nodeId, path);
    updateDirectoryTabNodeState(tab, nodeId, nodeName);
    applyNodeTabPathState(tab, nodeId, nodeName, path);
    refreshDirectoryTabOnlineState(tab, nodeId);
}

/**
 * @brief 将节点、路径和面包屑状态同步到目标目录页。
 * @param tab 目标页签。
 * @param nodeId 当前节点 ID。
 * @param nodeName 当前节点显示名。
 * @param path 当前目录路径。
 */
void DirectoryPageController::applyNodeTabPathState(QWidget *tab,
                                                    const QString &nodeId,
                                                    const QString &nodeName,
                                                    const QString &path)
{
    if (!tab || !m_fileBrowser) {
        return;
    }

    tab->setProperty("nodeId", nodeId);
    tab->setProperty("dirPath", path);

    if (QLineEdit *pathEdit = tab->findChild<QLineEdit*>("pathEdit")) {
        pathEdit->setText(path);
    }

    if (QTreeWidget *folderTree = tab->findChild<QTreeWidget*>("folderTree")) {
        m_fileBrowser->loadFileList(nodeId, path, folderTree);
    }

    if (QLayout *breadcrumbItemsLayout = tab->findChild<QLayout*>("breadcrumbItemsLayout")) {
        m_directoryModule->updateBreadcrumbNavigation(breadcrumbItemsLayout, nodeId, path);
    }

    if (m_nodeTabWidget) {
        const int tabIndex = m_nodeTabWidget->indexOf(tab);
        if (tabIndex >= 0) {
            m_nodeTabWidget->setTabText(tabIndex, DirectoryTabPresentation::title(nodeName, path));
        }
    }
}

/**
 * @brief 为新建目录页签补齐固定 UI 骨架和初始上下文。
 * @param tab 新建页签对象。
 * @param nodeId 当前节点 ID。
 * @param nodeName 当前节点显示名。
 * @param dirPath 当前目录路径。
 */
void DirectoryPageController::setupDirectoryTabUi(QWidget *tab,
                                                  const QString &nodeId,
                                                  const QString &nodeName,
                                                  const QString &dirPath)
{
    if (!tab || !m_nodeTabWidget || !m_nodeGateway) {
        return;
    }

    tab->setProperty("nodeId", nodeId);
    tab->setProperty("dirPath", dirPath);
    if (QComboBox *filterComboBox = tab->findChild<QComboBox*>("filterComboBox")) {
        filterComboBox->clear();
        filterComboBox->addItem(QString::fromUtf8(u8"全部文件"));
        filterComboBox->addItem(QStringLiteral("仅目录"));
        filterComboBox->addItem(QString::fromUtf8(u8"文本文件"));
        filterComboBox->addItem(QString::fromUtf8(u8"图片文件"));
        filterComboBox->addItem(QString::fromUtf8(u8"音频文件"));
        filterComboBox->addItem(QString::fromUtf8(u8"视频文件"));
        filterComboBox->addItem(QString::fromUtf8(u8"压缩文件"));
        filterComboBox->addItem(QStringLiteral("可执行文件"));
    }

    if (QTreeWidget *folderTree = tab->findChild<QTreeWidget*>("folderTree")) {
        m_fileBrowser->configureFileTree(folderTree);
    }

    if (QLabel *nodeNameLabel = tab->findChild<QLabel*>("nodeNameLabel")) {
        nodeNameLabel->setText(nodeName);
    }

    if (QLineEdit *pathEdit = tab->findChild<QLineEdit*>("pathEdit")) {
        pathEdit->setText(dirPath);
    }

    const int tabIndex = m_nodeTabWidget->addTab(tab, DirectoryTabPresentation::title(nodeName, dirPath));
    m_nodeTabWidget->setCurrentIndex(tabIndex);
    updateDirectoryTabNodeState(tab, nodeId, nodeName);
}

/**
 * @brief 绑定目录页签内部的搜索、筛选、导航和同步交互。
 * @param tab 目标目录页签。
 */
void DirectoryPageController::bindDirectoryTabInteractions(QWidget *tab)
{
    if (!tab || !m_downloadNodeComboBox || !m_directoryModule || !m_nodeGateway || !m_directoryGateway) {
        return;
    }

    QLineEdit *pathEdit = tab->findChild<QLineEdit*>("pathEdit");
    QLabel *nodeNameLabel = tab->findChild<QLabel*>("nodeNameLabel");
    QLabel *nodeStatusLabel = tab->findChild<QLabel*>("nodeStatusLabel");
    QTreeWidget *folderTree = tab->findChild<QTreeWidget*>("folderTree");
    QLineEdit *searchEdit = tab->findChild<QLineEdit*>("searchEdit");
    QPushButton *searchButton = tab->findChild<QPushButton*>("searchButton");
    QComboBox *filterComboBox = tab->findChild<QComboBox*>("filterComboBox");
    QPushButton *upButton = tab->findChild<QPushButton*>("upButton");
    QPushButton *browseButton = tab->findChild<QPushButton*>("browseButton");
    QPushButton *refreshButton = tab->findChild<QPushButton*>("refreshButton");
    QCheckBox *autoSyncCheckBox = tab->findChild<QCheckBox*>("autoSyncCheckBox");

    connect(upButton, &QPushButton::clicked, this, [=]() {
        if (!pathEdit) {
            return;
        }
        const QString currentPath = pathEdit->text();
        if (currentPath == "/") {
            return;
        }
        const int lastSlashIndex = currentPath.lastIndexOf("/");
        const QString nextPath = lastSlashIndex > 0 ? currentPath.left(lastSlashIndex) : "/";
        navigateDirectoryTab(tab,
                             directoryTabNodeId(tab),
                             directoryTabNodeName(tab, nodeNameLabel ? nodeNameLabel->text() : QString()),
                             nextPath);
    });

    connect(browseButton, &QPushButton::clicked, this, [=]() {
        if (!pathEdit) {
            return;
        }
        DirectorySelectionResult selection;
        const int nodeIndex = m_downloadNodeComboBox->findData(directoryTabNodeId(tab));
        if (!openDirectorySelectionDialog(nodeIndex, pathEdit->text(), selection)) {
            return;
        }

        QString selectedNodeName = directoryTabNodeName(tab, nodeNameLabel ? nodeNameLabel->text() : QString());
        const int selectedNodeIndex = m_downloadNodeComboBox->findData(selection.nodeId);
        if (selectedNodeIndex >= 0) {
            selectedNodeName = m_downloadNodeComboBox->itemText(selectedNodeIndex);
            if (nodeNameLabel) {
                nodeNameLabel->setText(selectedNodeName);
            }
        }

        if (nodeStatusLabel) {
            const NodeGateway::NodeInfo selectedNodeInfo = m_nodeGateway->nodeInfo(selection.nodeId);
            const bool selectedNodeOnline = (selectedNodeInfo.status == 1);
            nodeStatusLabel->setText(selectedNodeOnline ? QStringLiteral("在线") : QStringLiteral("离线"));
            nodeStatusLabel->setStyleSheet(selectedNodeOnline
                ? "color: #07856F; font-size: 13px; font-weight: 600;"
                : "color: #C73B52; font-size: 13px; font-weight: 600;");
        }

        navigateDirectoryTab(tab, selection.nodeId, selectedNodeName, selection.path);
    });

    connect(refreshButton, &QPushButton::clicked, this, [=]() {
        if (!pathEdit) {
            return;
        }
        navigateDirectoryTab(tab,
                             directoryTabNodeId(tab),
                             directoryTabNodeName(tab, nodeNameLabel ? nodeNameLabel->text() : QString()),
                             pathEdit->text());
    });

    connect(autoSyncCheckBox, &QCheckBox::checkStateChanged, this, [=](Qt::CheckState state) {
        const QString currentNodeId = directoryTabNodeId(tab);
        if (state == Qt::Checked) {
            m_directoryGateway->startFileListSync(currentNodeId, 10000);
        } else {
            m_directoryGateway->stopFileListSync(currentNodeId);
        }
    });

    connect(folderTree, &QTreeWidget::itemDoubleClicked, this, [=](QTreeWidgetItem *item, int) {
        if (!item || !item->data(1, Qt::UserRole).toBool()) {
            return;
        }
        navigateDirectoryTab(tab,
                             directoryTabNodeId(tab),
                             directoryTabNodeName(tab, nodeNameLabel ? nodeNameLabel->text() : QString()),
                             item->data(0, Qt::UserRole).toString());
    });

    connect(searchEdit, &QLineEdit::returnPressed, searchButton, &QPushButton::click);
    connect(searchButton, &QPushButton::clicked, this, [=]() {
        m_directoryModule->handleSearch(tab, m_messageParent);
    });
    connect(filterComboBox, &QComboBox::currentIndexChanged, this, [=](int) {
        m_directoryModule->handleFilter(tab);
    });
}

void DirectoryPageController::bindDirectoryInteractions(QWidget *tab)
{
    bindDirectoryTabInteractions(tab);
}

QWidget *DirectoryPageController::createNodeDirectoryTab(const QString &nodeId,
                                                         const QString &nodeName,
                                                         const QString &dirPath)
{
    DirectoryTabPage *newTab = new DirectoryTabPage(m_nodeTabWidget);
    setupDirectoryTabUi(newTab, nodeId, nodeName, dirPath);
    bindDirectoryTabInteractions(newTab);
    navigateDirectoryTab(newTab, nodeId, nodeName, dirPath);
    return newTab;
}

void DirectoryPageController::handleBreadcrumbNavigate(const QString &nodeId, const QString &path)
{
    if (!m_fileBrowser || !m_nodeTabWidget) {
        return;
    }

    m_fileBrowser->setCurrentPath(nodeId, path);
    QWidget *currentTab = m_nodeTabWidget->currentWidget();
    if (!currentTab) {
        return;
    }

    currentTab->setProperty("nodeId", nodeId);
    const QString currentNodeName = DirectoryTabPresentation::nodeName(
        m_nodeTabWidget->tabText(m_nodeTabWidget->currentIndex()));
    applyNodeTabPathState(currentTab, nodeId, currentNodeName, path);
}
