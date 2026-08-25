/**
 * @file TaskCreationGateway.cpp
 * @brief 任务创建窄端口的适配实现。
 *
 * 这里不启动传输、不维护任务状态，只把输入参数原样交给 TaskManager
 * 并把返回的 taskId 传回上传/下载模块。
 */
#include "TaskCreationGateway.h"

#include "TaskManager.h"

TaskCreationGateway::TaskCreationGateway(TaskManager *taskManager, QObject *parent)
    : QObject(parent)
    , m_taskManager(taskManager)
{
}

bool TaskCreationGateway::isAvailable() const
{
    return m_taskManager != nullptr;
}

QString TaskCreationGateway::createUploadTask(const QString &localPath, const QString &nodeId) const
{
    return m_taskManager ? m_taskManager->addUploadTask(localPath, nodeId) : QString();
}

QString TaskCreationGateway::createDownloadTask(const QString &remoteFilePath,
                                                const QString &savePath,
                                                const QString &nodeId,
                                                qint64 fileSize) const
{
    return m_taskManager
        ? m_taskManager->addDownloadTask(remoteFilePath, savePath, nodeId, fileSize)
        : QString();
}
