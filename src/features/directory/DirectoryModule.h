#ifndef DIRECTORYMODULE_H
#define DIRECTORYMODULE_H

#include <QObject>
#include <QLayout>
#include <QTabWidget>
#include <QTreeWidget>

class QWidget;

// 目录模块本体：负责目录搜索、过滤、面包屑和页签交互规则；它仍直接操作目录页控件，
// 但不持有网络服务或跨页持久化状态。
class DirectoryModule : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryModule(QObject *parent = nullptr);

    int searchDirectoryTree(QTreeWidget *tree, const QString &searchText) const;
    void applyDirectoryFilter(QTreeWidget *tree, const QString &filterType) const;
    void updateBreadcrumbNavigation(QLayout *breadcrumbLayout,
                                    const QString &nodeId,
                                    const QString &path);

    void handleSearch(QWidget *tab, QWidget *messageParent) const;
    void handleFilter(QWidget *tab) const;
    bool navigateToParent(QWidget *tab,
                          const QTabWidget *tabWidget,
                          QString &nodeId,
                          QString &nodeName,
                          QString &path) const;
    bool refreshCurrentDirectory(QWidget *tab,
                                 const QTabWidget *tabWidget,
                                 QString &nodeId,
                                 QString &nodeName,
                                 QString &path) const;
    QString currentTabNodeId(QWidget *tab, const QString &fallbackNodeId = QString()) const;
    QString currentTabNodeName(QWidget *tab,
                               const QTabWidget *tabWidget,
                               const QString &fallbackNodeName = QString()) const;

signals:
    void navigateToPath(const QString &nodeId, const QString &path);

private:
    static bool matchesSearchText(const QString &itemText, const QString &searchText);
    static bool matchesFilterType(const QString &fileName,
                                  bool isDirectory,
                                  const QString &filePath,
                                  const QString &filterType);
    static int searchDirectoryTreeRecursive(QTreeWidgetItem *item, const QString &searchText, QTreeWidget *tree);
    static bool applyDirectoryFilterRecursive(QTreeWidgetItem *item, const QString &filterType);
};

#endif // DIRECTORYMODULE_H
