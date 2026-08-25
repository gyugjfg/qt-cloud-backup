#ifndef NODESERVICE_H
#define NODESERVICE_H

#include "NetworkTypes.h"

#include <QObject>
#include <QMap>
#include <QMutex>

class QTcpSocket;

// 节点能力层：负责节点元数据、连接复用和在线状态探测。
class NodeService : public QObject
{
    Q_OBJECT

public:
    /** 创建节点服务；服务对象只维护网络缓存和连接，不操作 QWidget。 */
    explicit NodeService(QObject *parent = nullptr);
    ~NodeService();

    /** 新增节点并建立可供后续请求使用的元数据。 */
    bool addNode(const NetworkNodeInfo &node);
    /** 删除节点并断开其已缓存连接。 */
    bool removeNode(const QString &nodeId);
    /** 仅更新内存中的节点元数据；已有 socket 不会因本次更新自动重建。 */
    bool updateNode(const NetworkNodeInfo &node);
    /** 返回线程安全的节点快照列表。 */
    QList<NetworkNodeInfo> getNodeList() const;
    /** 返回单个节点快照；不存在时返回默认值。 */
    NetworkNodeInfo getNodeInfo(const QString &nodeId) const;
    /** 同步探测节点是否可连接，并更新在线状态。 */
    bool checkNodeStatus(const QString &nodeId);
    /** 在线程/事件队列中执行探测，并通过 nodeStatusChecked 回传。 */
    void checkNodeStatusAsync(const QString &nodeId);
    /** 获取或建立节点 TCP 连接；返回指针由 NodeService 持有，调用方不得删除或跨线程操作。 */
    QTcpSocket *getConnection(const QString &nodeId);

signals:
    /** 异步在线探测完成后的值对象结果。 */
    void nodeStatusChecked(const QString &nodeId, bool online);

private:
    bool connectToNode(const NetworkNodeInfo &node);
    void disconnectFromNode(const QString &nodeId);

    QMap<QString, NetworkNodeInfo> m_nodes;
    QMap<QString, QTcpSocket*> m_connections;
    mutable QMutex m_nodesMutex;
    mutable QMutex m_connectionsMutex;
};

#endif // NODESERVICE_H
