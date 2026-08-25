#ifndef NODEGATEWAY_H
#define NODEGATEWAY_H

#include "NetworkTypes.h"

#include <QObject>

class NetWork;

// 节点能力的窄适配器，隔离页面模块对 NetWork 宽门面的直接依赖。
class NodeGateway : public QObject
{
    Q_OBJECT

public:
    using NodeInfo = NetworkNodeInfo;

    // Gateway 只借用 NetWork，不负责其生命周期；通常由 HomeWidge 作为父对象管理。
    explicit NodeGateway(NetWork *network, QObject *parent = nullptr);

    /** 将节点写入网络缓存和持久化前的协调入口；virtual 便于注入测试替身。 */
    virtual bool addNode(const NodeInfo &node);
    /** 按 nodeId 删除节点，并让下游停止该节点的相关网络活动；virtual 便于注入测试替身。 */
    virtual bool removeNode(const QString &nodeId);
    /** 更新节点元数据，返回下游是否接受本次更新；virtual 便于注入测试替身。 */
    virtual bool updateNode(const NodeInfo &node);
    /** 返回当前网络层维护的节点快照，不返回内部容器引用；virtual 便于注入测试替身。 */
    virtual QList<NodeInfo> nodeList() const;
    /** 查询单个节点；节点不存在时返回默认构造的 NodeInfo。virtual 便于注入测试替身。 */
    virtual NodeInfo nodeInfo(const QString &nodeId) const;
    /** 同步探测节点在线状态；耗时探测不应在 UI 线程直接调用，virtual 便于注入测试替身。 */
    virtual bool checkNodeStatus(const QString &nodeId);

private:
    NetWork *m_network;
};

#endif // NODEGATEWAY_H
