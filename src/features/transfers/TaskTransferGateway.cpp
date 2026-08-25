/**
 * @file TaskTransferGateway.cpp
 * @brief 任务传输窄端口适配实现。
 *
 * Gateway 只负责 NetWork 的请求转发和信号桥接。它不等待网络结果，
 * 也不把错误转换为任务终态；状态归并仍由 TaskManager 完成。
 */
#include "TaskTransferGateway.h"
#include "NetWork.h"

TaskTransferGateway::TaskTransferGateway(NetWork *network, QObject *parent)
    : QObject(parent), m_network(network)
{
    if (!m_network) {
        return;
    }

    connect(m_network, &NetWork::taskProgressChanged,
            this, &TaskTransferGateway::taskProgressChanged);
    connect(m_network, &NetWork::taskStatusChanged,
            this, &TaskTransferGateway::taskStatusChanged);
    connect(m_network, &NetWork::taskError,
            this, &TaskTransferGateway::taskError);
}

bool TaskTransferGateway::isAvailable() const
{
    return m_network != nullptr;
}

void TaskTransferGateway::startTransfer(const TransferRequest &request)
{
    if (m_network) {
        m_network->StartTransferAsync(request);
    }
}

void TaskTransferGateway::controlTransfer(const QString &taskId,
                                          TransferType type,
                                          TransferControlAction action)
{
    if (m_network) {
        m_network->ControlTransfer(taskId, type, action);
    }
}
