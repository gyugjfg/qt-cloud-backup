/*
 * 节点网络服务实现：维护节点元数据和可复用 TCP 连接。
 * 线程安全边界由节点锁与连接锁共同保护，调用方只接收值对象或 Qt 信号。
 */
#include "NodeService.h"

#include <QDebug>
#include <QMetaObject>
#include <QMutexLocker>
#include <QPointer>
#include <QTcpSocket>
#include <QThreadPool>

NodeService::NodeService(QObject *parent)
    : QObject(parent)
{
    // 节点连接统一沉到服务层，避免目录、上传、下载各自维护连接状态。
}

NodeService::~NodeService()
{
    const QList<QString> nodeIds = m_connections.keys();
    for (const QString &nodeId : nodeIds) {
        disconnectFromNode(nodeId);
    }
}

bool NodeService::addNode(const NetworkNodeInfo &node)
{
    QMutexLocker locker(&m_nodesMutex);
    if (m_nodes.contains(node.nodeId)) {
        return false;
    }
    m_nodes[node.nodeId] = node;
    return true;
}

bool NodeService::removeNode(const QString &nodeId)
{
    {
        QMutexLocker locker(&m_nodesMutex);
        if (!m_nodes.contains(nodeId)) {
            return false;
        }
        m_nodes.remove(nodeId);
    }

    disconnectFromNode(nodeId);
    return true;
}

bool NodeService::updateNode(const NetworkNodeInfo &node)
{
    QMutexLocker locker(&m_nodesMutex);
    if (!m_nodes.contains(node.nodeId)) {
        return false;
    }
    m_nodes[node.nodeId] = node;
    return true;
}

QList<NetworkNodeInfo> NodeService::getNodeList() const
{
    QMutexLocker locker(&m_nodesMutex);
    return m_nodes.values();
}

NetworkNodeInfo NodeService::getNodeInfo(const QString &nodeId) const
{
    QMutexLocker locker(&m_nodesMutex);
    return m_nodes.value(nodeId);
}

/**
 * @brief 同步检查指定节点当前是否在线，并回写节点缓存状态。
 * @param nodeId 目标节点 ID。
 * @return 节点当前是否在线。
 */
bool NodeService::checkNodeStatus(const QString &nodeId)
{
    NetworkNodeInfo node;
    {
        QMutexLocker locker(&m_nodesMutex);
        if (!m_nodes.contains(nodeId)) {
            return false;
        }
        node = m_nodes.value(nodeId);
    }

    bool ok = false;
    {
        QMutexLocker locker(&m_connectionsMutex);
        QTcpSocket *existingSocket = m_connections.value(nodeId, nullptr);
        ok = existingSocket && existingSocket->state() == QAbstractSocket::ConnectedState;
    }

    if (!ok) {
        QTcpSocket probeSocket;
        probeSocket.connectToHost(node.ip, node.port);
        ok = probeSocket.waitForConnected(1500);
        if (ok) {
            probeSocket.disconnectFromHost();
        }
    }

    {
        QMutexLocker locker(&m_nodesMutex);
        if (m_nodes.contains(nodeId)) {
            NetworkNodeInfo updatedNode = m_nodes.value(nodeId);
            updatedNode.status = ok ? 1 : 0;
            m_nodes[nodeId] = updatedNode;
        }
    }
    return ok;
}

/**
 * @brief 异步检查指定节点在线状态，并通过信号回传结果。
 * @param nodeId 目标节点 ID。
 */
void NodeService::checkNodeStatusAsync(const QString &nodeId)
{
    QPointer<NodeService> serviceGuard(this);
    NetworkNodeInfo node;
    {
        QMutexLocker locker(&m_nodesMutex);
        if (!m_nodes.contains(nodeId)) {
            QMetaObject::invokeMethod(serviceGuard.data(), [serviceGuard, nodeId]() {
                if (!serviceGuard) {
                    return;
                }
                emit serviceGuard->nodeStatusChecked(nodeId, false);
            }, Qt::QueuedConnection);
            return;
        }
        node = m_nodes.value(nodeId);
    }

    bool hasConnection = false;
    {
        QMutexLocker locker(&m_connectionsMutex);
        QTcpSocket *existingSocket = m_connections.value(nodeId, nullptr);
        hasConnection = existingSocket && existingSocket->state() == QAbstractSocket::ConnectedState;
    }

    // 已有可用连接时直接复用结果，避免每次都重新做探测。
    if (hasConnection) {
        QMetaObject::invokeMethod(serviceGuard.data(), [serviceGuard, nodeId]() {
            if (!serviceGuard) {
                return;
            }
            emit serviceGuard->nodeStatusChecked(nodeId, true);
        }, Qt::QueuedConnection);
        return;
    }

    // worker 只使用节点值快照；缓存写入和信号发送回到受守卫的服务线程。
    QThreadPool::globalInstance()->start([serviceGuard, nodeId, node]() {
        QTcpSocket probeSocket;
        probeSocket.connectToHost(node.ip, node.port);
        bool ok = probeSocket.waitForConnected(1500);
        if (ok) {
            probeSocket.disconnectFromHost();
        }

        if (!serviceGuard) {
            return;
        }

        QMetaObject::invokeMethod(serviceGuard.data(), [serviceGuard, nodeId, ok]() {
            if (!serviceGuard) {
                return;
            }

            {
                QMutexLocker locker(&serviceGuard->m_nodesMutex);
                if (serviceGuard->m_nodes.contains(nodeId)) {
                    NetworkNodeInfo updatedNode = serviceGuard->m_nodes.value(nodeId);
                    updatedNode.status = ok ? 1 : 0;
                    serviceGuard->m_nodes[nodeId] = updatedNode;
                }
            }

            emit serviceGuard->nodeStatusChecked(nodeId, ok);
        }, Qt::QueuedConnection);
    });
}

QTcpSocket *NodeService::getConnection(const QString &nodeId)
{
    const NetworkNodeInfo node = getNodeInfo(nodeId);
    if (node.nodeId.isEmpty()) {
        return nullptr;
    }

    {
        QMutexLocker locker(&m_connectionsMutex);
        if (m_connections.contains(nodeId)) {
            QTcpSocket *socket = m_connections.value(nodeId);
            if (socket && socket->state() == QTcpSocket::ConnectedState) {
                return socket;
            }
            delete socket;
            m_connections.remove(nodeId);
        }
    }

    if (connectToNode(node)) {
        QMutexLocker locker(&m_connectionsMutex);
        return m_connections.value(nodeId, nullptr);
    }
    return nullptr;
}

/**
 * @brief 为指定节点建立可复用连接。
 * @param node 目标节点信息。
 * @return 连接是否成功建立。
 */
bool NodeService::connectToNode(const NetworkNodeInfo &node)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->connectToHost(node.ip, node.port);
    if (!socket->waitForConnected(10000)) {
        qWarning() << "[NodeService] 连接节点失败:" << node.nodeName
                   << "(" << node.ip << ":" << node.port << ")"
                   << "错误:" << socket->errorString();
        delete socket;
        return false;
    }

    QMutexLocker locker(&m_connectionsMutex);
    m_connections[node.nodeId] = socket;
    return true;
}

void NodeService::disconnectFromNode(const QString &nodeId)
{
    QTcpSocket *socket = nullptr;
    {
        QMutexLocker locker(&m_connectionsMutex);
        if (m_connections.contains(nodeId)) {
            socket = m_connections.take(nodeId);
        }
    }

    if (socket) {
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(1000);
        }
        delete socket;
    }
}
