/*
 * NetWork 兼容门面实现：把旧的统一入口映射到节点、目录和传输服务。
 * 这里允许存在兼容包装，但不把具体 socket、SQLite 或页面逻辑重新塞回门面。
 */
#include "NetWork.h"

#include "DirectoryService.h"
#include "NodeService.h"
#include "TransferService.h"

#include <QDebug>
#include <QMetaObject>

NetWork::NetWork(QObject *parent)
    : QObject(parent)
    , m_transferService(new TransferService(this))
    , m_nodeService(new NodeService(this))
    , m_directoryService(new DirectoryService(m_nodeService, this))
{
    connect(m_transferService, &TransferService::taskProgressChanged,
            this, &NetWork::sendTaskProgressSignal);
    connect(m_transferService, &TransferService::taskStatusChanged,
            this, &NetWork::sendTaskStatusSignal);
    connect(m_transferService, &TransferService::taskError,
            this, &NetWork::sendTaskErrorSignal);
    connect(m_nodeService, &NodeService::nodeStatusChecked,
            this, &NetWork::nodeStatusChecked);
    connect(m_directoryService, &DirectoryService::fileListUpdated,
            this, &NetWork::fileListUpdated);

}

NetWork::~NetWork() = default;

void NetWork::sendTaskProgressSignal(const QString &taskId, int progress,
                                     qint64 transferred, qint64 total, double speed)
{
    // 统一切回 NetWork 所在线程再发信号，避免页面层直接接收工作线程回调。
    QMetaObject::invokeMethod(this, [=]() {
        emit taskProgressChanged(taskId, progress, transferred, total, speed);
    }, Qt::QueuedConnection);
}

void NetWork::sendTaskStatusSignal(const QString &taskId, int status)
{
    QMetaObject::invokeMethod(this, [=]() {
        emit taskStatusChanged(taskId, status);
    }, Qt::QueuedConnection);
}

void NetWork::sendTaskErrorSignal(const QString &taskId, const QString &errorMessage)
{
    QMetaObject::invokeMethod(this, [=]() {
        emit taskError(taskId, errorMessage);
    }, Qt::QueuedConnection);
}

bool NetWork::AddNode(const NodeInfo &node)
{
    return m_nodeService->addNode(node);
}

bool NetWork::RemoveNode(const QString &nodeId)
{
    StopFileListSync(nodeId);
    return m_nodeService->removeNode(nodeId);
}

bool NetWork::UpdateNode(const NodeInfo &node)
{
    return m_nodeService->updateNode(node);
}

QList<NetWork::NodeInfo> NetWork::GetNodeList()
{
    return m_nodeService->getNodeList();
}

NetWork::NodeInfo NetWork::GetNodeInfo(const QString &nodeId) const
{
    return m_nodeService->getNodeInfo(nodeId);
}

bool NetWork::CheckNodeStatus(const QString &nodeId)
{
    return m_nodeService->checkNodeStatus(nodeId);
}

void NetWork::CheckNodeStatusAsync(const QString &nodeId)
{
    m_nodeService->checkNodeStatusAsync(nodeId);
}

TransferService::TransferRequest NetWork::toServiceRequest(const TransferRequest &request) const
{
    // NetWork 负责把外部统一请求补齐成底层可执行请求，调用方不用再关心节点连接细节。
    TransferService::TransferRequest serviceRequest;
    serviceRequest.type = request.type == TransferRequest::Type::Upload
        ? TransferService::TransferKind::Upload
        : TransferService::TransferKind::Download;
    serviceRequest.filePath = request.filePath;
    serviceRequest.fileName = request.fileName;
    serviceRequest.savePath = request.savePath;
    serviceRequest.nodeId = request.nodeId;
    serviceRequest.taskId = request.taskId;
    serviceRequest.threadCount = request.threadCount;
    serviceRequest.startOffset = request.startOffset;

    const NodeInfo node = GetNodeInfo(request.nodeId);
    serviceRequest.nodeIp = node.ip;
    serviceRequest.nodePort = node.port;
    return serviceRequest;
}

bool NetWork::FileUpload(QString filePath, const QString &nodeId, const QString &taskId, qint64 startOffset)
{
    // 旧入口继续保留给兼容调用方，内部仍然走同一份请求转换逻辑。
    TransferRequest request;
    request.type = TransferRequest::Type::Upload;
    request.filePath = filePath;
    request.nodeId = nodeId;
    request.taskId = taskId;
    request.startOffset = startOffset;
    return m_transferService->fileUpload(toServiceRequest(request));
}

void NetWork::FileUploadAsync(QString filePath, const QString &nodeId, const QString &taskId, qint64 startOffset)
{
    TransferRequest request;
    request.type = TransferRequest::Type::Upload;
    request.filePath = filePath;
    request.nodeId = nodeId;
    request.taskId = taskId;
    request.startOffset = startOffset;
    StartTransferAsync(request);
}

bool NetWork::FileDownload(QString fileName, QString filePath, const QString &nodeId,
                           const QString &taskId, int threadCount, qint64 startOffset)
{
    // 旧入口继续保留给兼容调用方，避免这轮注释整理顺手改动主线 API。
    TransferRequest request;
    request.type = TransferRequest::Type::Download;
    request.fileName = fileName;
    request.savePath = filePath;
    request.nodeId = nodeId;
    request.taskId = taskId;
    request.threadCount = threadCount;
    request.startOffset = startOffset;
    return m_transferService->fileDownload(toServiceRequest(request));
}

void NetWork::FileDownloadAsync(QString fileName, QString filePath, const QString &nodeId,
                                const QString &taskId, int threadCount, qint64 startOffset)
{
    TransferRequest request;
    request.type = TransferRequest::Type::Download;
    request.fileName = fileName;
    request.savePath = filePath;
    request.nodeId = nodeId;
    request.taskId = taskId;
    request.threadCount = threadCount;
    request.startOffset = startOffset;
    StartTransferAsync(request);
}

void NetWork::StartTransferAsync(const TransferRequest &request)
{
    // 统一启动入口只负责协调，实际排队和执行继续交给 TransferService。
    m_transferService->startTransferAsync(toServiceRequest(request));
}

void NetWork::ControlTransfer(const QString &taskId,
                              TransferRequest::Type type,
                              TransferControlAction action)
{
    // 暂停 / 取消统一在这里收口，任务层不再分散调用多套底层控制函数。
    m_transferService->controlTransfer(
        taskId,
        type == TransferRequest::Type::Upload
            ? TransferService::TransferKind::Upload
            : TransferService::TransferKind::Download,
        action == TransferControlAction::Pause
            ? TransferService::TransferControlAction::Pause
            : TransferService::TransferControlAction::Cancel);
}

QList<NetWork::FileInfo> NetWork::GetFileInfoList(const QString &nodeId, const QString &path)
{
    return m_directoryService->getFileInfoList(nodeId, path);
}

NetWork::FileInfo NetWork::GetFileInfo(const QString &nodeId, const QString &filePath)
{
    return m_directoryService->getFileInfo(nodeId, filePath);
}

void NetWork::StartFileListSync(const QString &nodeId, int interval)
{
    m_directoryService->startFileListSync(nodeId, interval);
}

void NetWork::StopFileListSync(const QString &nodeId)
{
    m_directoryService->stopFileListSync(nodeId);
}

bool NetWork::Command(NetWork::CMD cmd, QString name, const QString &nodeId)
{
    return m_directoryService->command(
        cmd == CREATE ? DirectoryService::DirectoryCommand::Create
                      : DirectoryService::DirectoryCommand::Delete,
        name,
        nodeId);
}

bool NetWork::ChangeDirectory(const QString &nodeId, const QString &path)
{
    return m_directoryService->changeDirectory(nodeId, path);
}

void NetWork::pauseDownload(const QString &taskId)
{
    // 兼容包装保留给旧调用方，内部统一落到 ControlTransfer。
    ControlTransfer(taskId, TransferRequest::Type::Download, TransferControlAction::Pause);
}

void NetWork::cancelDownload(const QString &taskId)
{
    ControlTransfer(taskId, TransferRequest::Type::Download, TransferControlAction::Cancel);
}

void NetWork::pauseUpload(const QString &taskId)
{
    ControlTransfer(taskId, TransferRequest::Type::Upload, TransferControlAction::Pause);
}

void NetWork::cancelUpload(const QString &taskId)
{
    ControlTransfer(taskId, TransferRequest::Type::Upload, TransferControlAction::Cancel);
}
