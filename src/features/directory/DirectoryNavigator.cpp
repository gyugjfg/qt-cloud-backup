#include "DirectoryNavigator.h"
#include "FileTypePolicy.h"

#include <QTreeWidgetItem>
#include <QFileInfo>
#include <QPushButton>
#include <QLabel>

DirectoryNavigator::DirectoryNavigator(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief 在当前目录树里查找匹配关键字的项。
 * @param tree 目标目录树。
 * @param searchText 搜索关键字。
 * @return 命中的目录项数量。
 */
int DirectoryNavigator::searchDirectoryTree(QTreeWidget *tree, const QString &searchText) const
{
    if (!tree || searchText.isEmpty()) {
        return 0;
    }

    int foundCount = 0;
    for (int i = 0; i < tree->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        if (item->text(0).contains(searchText, Qt::CaseInsensitive)) {
            item->setSelected(true);
            tree->scrollToItem(item);
            foundCount++;
        }
    }

    return foundCount;
}

/**
 * @brief 按给定筛选类型隐藏不匹配的目录项。
 * @param tree 目标目录树。
 * @param filterType 当前筛选类型文案。
 */
void DirectoryNavigator::applyDirectoryFilter(QTreeWidget *tree, const QString &filterType) const
{
    if (!tree) {
        return;
    }

    for (int i = 0; i < tree->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        const bool isDirectory = item->data(1, Qt::UserRole).toBool();
        const QString filePath = item->data(0, Qt::UserRole).toString();
        const QString fileName = filePath.isEmpty() ? item->text(0) : QFileInfo(filePath).fileName();
        const bool showItem = FileTypePolicy::matches(fileName, isDirectory, filterType, filePath);

        item->setHidden(!showItem);
    }
}

/**
 * @brief 重建面包屑导航按钮链。
 * @param breadcrumbLayout 面包屑布局。
 * @param nodeId 当前节点 ID。
 * @param path 当前目录路径。
 */
void DirectoryNavigator::updateBreadcrumbNavigation(QLayout *breadcrumbLayout,
                                                    const QString &nodeId,
                                                    const QString &path)
{
    if (!breadcrumbLayout) return;

    QLayoutItem *item;
    while ((item = breadcrumbLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    QPushButton *rootButton = new QPushButton("/");
    rootButton->setProperty("buttonRole", "ghost");
    breadcrumbLayout->addWidget(rootButton);

    connect(rootButton, &QPushButton::clicked, this, [=]() {
        emit navigateToPath(nodeId, "/");
    });

    if (path != "/") {
        QStringList pathParts = path.split("/");
        QString currentPath = "/";

        for (int i = 1; i < pathParts.size(); i++) {
            QString part = pathParts[i];
            if (part.isEmpty()) continue;

            currentPath += "/" + part;

            QLabel *separator = new QLabel("/");
            separator->setProperty("role", "muted");
            breadcrumbLayout->addWidget(separator);

            QPushButton *partButton = new QPushButton(part);
            partButton->setProperty("buttonRole", "ghost");
            breadcrumbLayout->addWidget(partButton);

            QString pathToSet = currentPath;
            connect(partButton, &QPushButton::clicked, this, [=]() {
                emit navigateToPath(nodeId, pathToSet);
            });
        }
    }
}
