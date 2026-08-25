/* 节点 Gateway 实现：只转发节点增删改查和在线探测，不扩散网络门面。 */
#include "NodeGateway.h"

#include "NetWork.h"

NodeGateway::NodeGateway(NetWork *network, QObject *parent)
    : QObject(parent), m_network(network)
{
}

bool NodeGateway::addNode(const NodeInfo &node)
{
    return m_network ? m_network->AddNode(node) : false;
}

bool NodeGateway::removeNode(const QString &nodeId)
{
    return m_network ? m_network->RemoveNode(nodeId) : false;
}

bool NodeGateway::updateNode(const NodeInfo &node)
{
    return m_network ? m_network->UpdateNode(node) : false;
}

QList<NodeGateway::NodeInfo> NodeGateway::nodeList() const
{
    return m_network ? m_network->GetNodeList() : QList<NodeInfo>();
}

NodeGateway::NodeInfo NodeGateway::nodeInfo(const QString &nodeId) const
{
    return m_network ? m_network->GetNodeInfo(nodeId) : NodeInfo();
}

bool NodeGateway::checkNodeStatus(const QString &nodeId)
{
    return m_network ? m_network->CheckNodeStatus(nodeId) : false;
}
