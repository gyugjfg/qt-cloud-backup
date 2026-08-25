#ifndef NODEMODULE_H
#define NODEMODULE_H

#include "NetworkTypes.h"

#include <QObject>
#include <QListWidget>
#include <QComboBox>
#include <QMap>

class Database;
class NodeGateway;
class NodeItem;
class NodeDialog;
class QListWidgetItem;
class QWidget;

/**
 * @brief 节点业务模块，协调节点持久化、网络缓存和节点选择控件。
 *
 * NodeModule 借用组合根注入的 Database、NodeGateway 和 Qt 控件，不拥有
 * 它们的生命周期。公开方法均应在 GUI 线程调用；数据库和 Gateway 的
 * 失败通过 false/空输出或不发起同步来表示，模块不会抛出异常。
 * 默认节点页的标题、路径和目录树展示由 NodePageController 负责。
 */
class NodeModule : public QObject
{
    Q_OBJECT

public:
    explicit NodeModule(Database *db, NodeGateway *nodeGateway, QObject *parent = nullptr);

    /** 注入节点列表和上传/下载选择器；控件由外部 UI 组合根拥有。 */
    void setUiComponents(QListWidget *nodeListWidget,
                         QComboBox *downloadNodeCombo,
                         QComboBox *uploadNodeCombo);

    /** 从数据库重建列表、选择器和 Gateway 缓存；依赖不完整时安全返回。 */
    void loadNodesFromDatabase();
    /** 将一条节点记录追加到列表，并同步其勾选索引。 */
    void appendNodeListItem(const NetworkNodeInfo &node, int itemIndex = -1);
    /** 将节点字段写入一个由列表层创建的行控件。 */
    void applyNodeDataToItem(NodeItem *nodeItem, const NetworkNodeInfo &node);
    /** 将节点追加到上传和下载选择器。 */
    void addNodeToSelectors(const NetworkNodeInfo &node);
    /** 在两个选择器中同步已有节点的展示名称。 */
    void syncNodeSelectors(const QString &nodeId, const QString &nodeName);
    /** 从两个选择器删除指定节点；找不到时不产生副作用。 */
    void removeNodeFromSelectors(const QString &nodeId);
    /** 查找行控件对应的 QListWidgetItem；找不到时返回 nullptr。 */
    QListWidgetItem *findNodeListItem(const NodeItem *nodeItem) const;
    /** 仅当恰好选中一条节点时返回其列表项，否则返回 nullptr。 */
    QListWidgetItem *selectedSingleNodeListItem() const;
    /** 重新编号行控件并重建勾选缓存。 */
    void refreshNodeItemIndices();
    /** 批量设置勾选状态，并发送一次汇总信号。 */
    void setAllNodesChecked(bool checked);
    /** 弹出编辑框并创建节点；成功时写入 createdNode。 */
    bool createNode(QWidget *dialogParent, NetworkNodeInfo &createdNode);
    /** 删除当前勾选节点；输出实际删除 ID，并返回是否删除当前节点。 */
    bool removeSelectedNodes(const QString &currentNodeId, QList<QString> &removedNodeIds);
    /** 编辑当前唯一选中节点；成功时写入 updatedNode。 */
    bool updateSelectedNode(QWidget *dialogParent, NetworkNodeInfo &updatedNode);
    /** 返回勾选缓存的副本；其中 QWidget 指针仍由列表控件非拥有地持有。 */
    QMap<int, QWidget*> checkedNodeItems() const { return m_checkNodeItems; }

signals:
    void nodesLoaded();
    void checkedNodeItemsChanged(int checkedCount, int totalCount);

private:
    Database *m_db;              ///< 借用的数据库对象，不由本模块释放。
    NodeGateway *m_nodeGateway;  ///< 借用的节点 Gateway，不由本模块释放。
    QListWidget *m_nodeListWidget = nullptr;
    QComboBox *m_downloadNodeCombo = nullptr;
    QComboBox *m_uploadNodeCombo = nullptr;
    QMap<int, QWidget*> m_checkNodeItems;

};

#endif // NODEMODULE_H
