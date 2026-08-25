/* 目录 Gateway 实现：把 NetWork 的目录接口收窄为目录 Feature 可消费的能力。 */
#include "DirectoryGateway.h"

#include "NetWork.h"

DirectoryGateway::DirectoryGateway(NetWork *network, QObject *parent)
    : QObject(parent), m_network(network)
{
    if (m_network) {
        connect(m_network, &NetWork::fileListUpdated,
                this, &DirectoryGateway::fileListUpdated);
    }
}

QList<DirectoryGateway::FileInfo> DirectoryGateway::fileInfoList(const QString &nodeId,
                                                                  const QString &path)
{
    return m_network ? m_network->GetFileInfoList(nodeId, path) : QList<FileInfo>();
}

DirectoryGateway::FileInfo DirectoryGateway::fileInfo(const QString &nodeId,
                                                       const QString &filePath)
{
    return m_network ? m_network->GetFileInfo(nodeId, filePath) : FileInfo();
}

void DirectoryGateway::startFileListSync(const QString &nodeId, int interval)
{
    if (m_network) {
        m_network->StartFileListSync(nodeId, interval);
    }
}

void DirectoryGateway::stopFileListSync(const QString &nodeId)
{
    if (m_network) {
        m_network->StopFileListSync(nodeId);
    }
}
