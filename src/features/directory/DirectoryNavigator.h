#ifndef DIRECTORYNAVIGATOR_H
#define DIRECTORYNAVIGATOR_H

#include <QObject>
#include <QTreeWidget>
#include <QLayout>

// 旧的目录导航辅助对象，当前更多保留给面包屑和基础搜索/筛选能力。
class DirectoryNavigator : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryNavigator(QObject *parent = nullptr);

    /**
     * @brief 在当前目录树中执行关键字搜索。
     * @param tree 目标目录树。
     * @param searchText 搜索关键字。
     * @return 命中的目录项数量。
     */
    int searchDirectoryTree(QTreeWidget *tree, const QString &searchText) const;
    /**
     * @brief 按给定筛选类型隐藏不匹配的目录项。
     * @param tree 目标目录树。
     * @param filterType 当前筛选类型文案。
     */
    void applyDirectoryFilter(QTreeWidget *tree, const QString &filterType) const;
    /**
     * @brief 重建面包屑导航按钮链。
     * @param breadcrumbLayout 面包屑布局。
     * @param nodeId 当前节点 ID。
     * @param path 当前目录路径。
     */
    void updateBreadcrumbNavigation(QLayout *breadcrumbLayout,
                                     const QString &nodeId,
                                     const QString &path);

signals:
    void navigateToPath(const QString &nodeId, const QString &path);
};

#endif // DIRECTORYNAVIGATOR_H
