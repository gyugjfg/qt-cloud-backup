/**
 * @file TaskNodeNameGateway.cpp
 * @brief 任务页节点名称查询适配实现。
 *
 * 查询保持同步且只读；NodeGateway 不可用或节点不存在时返回空名称，
 * 不在任务模块内补造默认节点数据。
 */
#include "TaskNodeNameGateway.h"

#include "NodeGateway.h"

TaskNodeNameGateway::TaskNodeNameGateway(NodeGateway *nodeGateway, QObject *parent)
    : QObject(parent), m_nodeGateway(nodeGateway)
{
}

QString TaskNodeNameGateway::nodeName(const QString &nodeId) const
{
    return m_nodeGateway ? m_nodeGateway->nodeInfo(nodeId).nodeName : QString();
}
