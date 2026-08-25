#include "DirectoryModule.h"
#include "DirectoryPathNavigation.h"
#include "FileTypePolicy.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidgetItem>

namespace {
QString normalizedText(const QString &text)
{
    return text.trimmed().toLower();
}
}

DirectoryModule::DirectoryModule(QObject *parent)
    : QObject(parent)
{
}

bool DirectoryModule::matchesSearchText(const QString &itemText, const QString &searchText)
{
    const QString normalizedItem = normalizedText(itemText);
    const QString normalizedSearch = normalizedText(searchText);
    return !normalizedSearch.isEmpty() && normalizedItem.contains(normalizedSearch);
}

bool DirectoryModule::matchesFilterType(const QString &fileName,
                                        bool isDirectory,
                                        const QString &filePath,
                                        const QString &filterType)
{
    return FileTypePolicy::matches(fileName, isDirectory, filterType, filePath);
}

int DirectoryModule::searchDirectoryTreeRecursive(QTreeWidgetItem *item, const QString &searchText, QTreeWidget *tree)
{
    if (!item || !tree) {
        return 0;
    }

    int foundCount = 0;
    const bool isMatch = !item->isHidden() && matchesSearchText(item->text(0), searchText);
    item->setSelected(isMatch);
    if (isMatch) {
        if (foundCount == 0) {
            tree->scrollToItem(item);
        }
        ++foundCount;
        QTreeWidgetItem *parent = item->parent();
        while (parent) {
            parent->setExpanded(true);
            parent = parent->parent();
        }
    }

    for (int i = 0; i < item->childCount(); ++i) {
        foundCount += searchDirectoryTreeRecursive(item->child(i), searchText, tree);
    }

    return foundCount;
}

bool DirectoryModule::applyDirectoryFilterRecursive(QTreeWidgetItem *item, const QString &filterType)
{
    if (!item) {
        return false;
    }

    const bool isDirectory = item->data(1, Qt::UserRole).toBool();
    const QString filePath = item->data(0, Qt::UserRole).toString();
    const QString fileName = filePath.isEmpty()
        ? item->text(0)
        : QFileInfo(filePath).fileName();
    const bool selfMatch = matchesFilterType(fileName, isDirectory, filePath, filterType);

    bool childMatch = false;
    for (int i = 0; i < item->childCount(); ++i) {
        childMatch = applyDirectoryFilterRecursive(item->child(i), filterType) || childMatch;
    }

    const bool showItem = selfMatch || childMatch;
    item->setHidden(!showItem);
    if (childMatch) {
        item->setExpanded(true);
    }
    return showItem;
}

int DirectoryModule::searchDirectoryTree(QTreeWidget *tree, const QString &searchText) const
{
    if (!tree) {
        return 0;
    }

    tree->clearSelection();
    if (searchText.trimmed().isEmpty()) {
        return 0;
    }

    int foundCount = 0;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        foundCount += searchDirectoryTreeRecursive(tree->topLevelItem(i), searchText, tree);
    }

    return foundCount;
}

void DirectoryModule::applyDirectoryFilter(QTreeWidget *tree, const QString &filterType) const
{
    if (!tree) {
        return;
    }

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        applyDirectoryFilterRecursive(tree->topLevelItem(i), filterType);
    }
}

/**
 * @brief 重建当前目录对应的面包屑按钮链。
 * @param breadcrumbLayout 面包屑按钮所在布局。
 * @param nodeId 当前节点 ID。
 * @param path 当前目录路径。
 */
void DirectoryModule::updateBreadcrumbNavigation(QLayout *breadcrumbLayout,
                                                 const QString &nodeId,
                                                 const QString &path)
{
    if (!breadcrumbLayout) {
        return;
    }

    QLayoutItem *item = nullptr;
    while ((item = breadcrumbLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    QPushButton *rootButton = new QPushButton("/");
    rootButton->setProperty("buttonRole", "ghost");
    breadcrumbLayout->addWidget(rootButton);

    connect(rootButton, &QPushButton::clicked, this, [this, nodeId]() {
        emit navigateToPath(nodeId, "/");
    });

    for (const DirectoryPathNavigation::Breadcrumb &breadcrumb
         : DirectoryPathNavigation::breadcrumbs(path)) {
        QLabel *separator = new QLabel("/");
        separator->setProperty("role", "muted");
        breadcrumbLayout->addWidget(separator);

        QPushButton *partButton = new QPushButton(breadcrumb.label);
        partButton->setProperty("buttonRole", "ghost");
        breadcrumbLayout->addWidget(partButton);

        const QString targetPath = breadcrumb.path;
        connect(partButton, &QPushButton::clicked, this, [this, nodeId, targetPath]() {
            emit navigateToPath(nodeId, targetPath);
        });
    }
}

QString DirectoryModule::currentTabNodeId(QWidget *tab, const QString &fallbackNodeId) const
{
    if (!tab) {
        return fallbackNodeId;
    }

    const QString nodeId = tab->property("nodeId").toString();
    return nodeId.isEmpty() ? fallbackNodeId : nodeId;
}

QString DirectoryModule::currentTabNodeName(QWidget *tab,
                                            const QTabWidget *tabWidget,
                                            const QString &fallbackNodeName) const
{
    if (!tab || !tabWidget) {
        return fallbackNodeName;
    }

    const int tabIndex = tabWidget->indexOf(tab);
    if (tabIndex < 0) {
        return fallbackNodeName;
    }

    const QString tabTitle = tabWidget->tabText(tabIndex);
    const QString nodeName = tabTitle.split(" - ").first();
    return nodeName.isEmpty() ? fallbackNodeName : nodeName;
}

/**
 * @brief 在当前页签中执行目录搜索，并给出搜索结果提示。
 * @param tab 当前目录页签。
 * @param messageParent 搜索结果提示框父对象。
 */
void DirectoryModule::handleSearch(QWidget *tab, QWidget *messageParent) const
{
    if (!tab) {
        return;
    }

    QTreeWidget *folderTree = tab->findChild<QTreeWidget*>("folderTree");
    QLineEdit *searchEdit = tab->findChild<QLineEdit*>("searchEdit");
    if (!folderTree || !searchEdit) {
        return;
    }

    const QString searchText = searchEdit->text();
    if (searchText.trimmed().isEmpty()) {
        return;
    }

    const int foundCount = searchDirectoryTree(folderTree, searchText);
    QMessageBox::information(messageParent,
                             QStringLiteral("搜索结果"),
                             foundCount > 0
                                 ? QStringLiteral("找到 %1 个匹配项").arg(foundCount)
                                 : QStringLiteral("未找到匹配的文件"));
}

void DirectoryModule::handleFilter(QWidget *tab) const
{
    if (!tab) {
        return;
    }

    QTreeWidget *folderTree = tab->findChild<QTreeWidget*>("folderTree");
    QComboBox *filterComboBox = tab->findChild<QComboBox*>("filterComboBox");
    if (!folderTree || !filterComboBox) {
        return;
    }

    applyDirectoryFilter(folderTree, filterComboBox->currentText());
}

/**
 * @brief 根据当前页签路径计算“返回上级目录”的目标上下文。
 * @param tab 当前目录页签。
 * @param tabWidget 目录页签所在容器。
 * @param nodeId 返回的目标节点 ID。
 * @param nodeName 返回的目标节点名。
 * @param path 返回的目标路径。
 * @return 是否成功得到有效的上级目录上下文。
 */
bool DirectoryModule::navigateToParent(QWidget *tab,
                                       const QTabWidget *tabWidget,
                                       QString &nodeId,
                                       QString &nodeName,
                                       QString &path) const
{
    if (!tab) {
        return false;
    }

    QLineEdit *pathEdit = tab->findChild<QLineEdit*>("pathEdit");
    if (!pathEdit) {
        return false;
    }

    const QString currentPath = pathEdit->text();
    if (!DirectoryPathNavigation::parentPath(currentPath, path)) {
        return false;
    }
    nodeId = currentTabNodeId(tab);
    nodeName = currentTabNodeName(tab, tabWidget);
    return !nodeId.isEmpty();
}

bool DirectoryModule::refreshCurrentDirectory(QWidget *tab,
                                              const QTabWidget *tabWidget,
                                              QString &nodeId,
                                              QString &nodeName,
                                              QString &path) const
{
    if (!tab) {
        return false;
    }

    QLineEdit *pathEdit = tab->findChild<QLineEdit*>("pathEdit");
    if (!pathEdit) {
        return false;
    }

    nodeId = currentTabNodeId(tab);
    nodeName = currentTabNodeName(tab, tabWidget);
    path = pathEdit->text();
    return !nodeId.isEmpty() && !path.isEmpty();
}
