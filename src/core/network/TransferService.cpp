/*
 * 传输执行器调度实现：负责请求校验、队列调度、生命周期和控制状态。
 * 文件协议与本地文件 I/O 位于 TransferServiceFileTransfer.cpp。
 */
#include "TransferService.h"

#include "TransferProtocolClient.h"
#include "TransferRequestPolicy.h"

#include <QDateTime>
#include <QMetaObject>
#include <QMutexLocker>
#include <QThread>
#include <QThreadPool>

#include <memory>

TransferService::TransferService(QObject *parent)
    : QObject(parent)
    , m_taskProcessing(false)
    , m_shutdownRequested(false)
{
    const int maxThreads = qMax(8, QThread::idealThreadCount() * 2);
    QThreadPool::globalInstance()->setMaxThreadCount(maxThreads);
}

TransferService::~TransferService()
{
    // 先阻止新任务，再关闭活动 socket；worker 会在现有控制检查点退出。
    m_shutdownRequested.store(true, std::memory_order_release);

    QMutexLocker queueLocker(&m_taskQueueMutex);
    m_taskQueue.clear();
    m_taskProcessing = false;
    queueLocker.unlock();

    {
        QMutexLocker controlLocker(&m_taskControlMutex);
        for (const int socket : m_transferControlState.activeSockets(TransferControlState::Direction::Download)) {
            TransferProtocolClient::closeSocket(socket);
        }
        for (const int socket : m_transferControlState.activeSockets(TransferControlState::Direction::Upload)) {
            TransferProtocolClient::closeSocket(socket);
        }
        m_transferControlState.clear();
    }

    // worker 捕获了 this，必须等其退出后才能完成 QObject 析构，避免悬空回调。
    QThreadPool::globalInstance()->waitForDone();
}

bool TransferService::fileUpload(const TransferRequest &request)
{
    return fileUploadResumable(request);
}

bool TransferService::fileDownload(const TransferRequest &request)
{
    return fileDownloadResumable(request);
}

void TransferService::startTransferAsync(const TransferRequest &request)
{
    if (m_shutdownRequested.load(std::memory_order_acquire)) {
        return;
    }

    // 异步传输统一先进队列，再由后台线程串行取出执行。
    enqueueTransferTask(request);
}

void TransferService::controlTransfer(const QString &taskId,
                                      TransferKind kind,
                                      TransferControlAction action)
{
    // 暂停和取消只在这里收口，避免调用方分别操心上传/下载两套控制入口。
    const bool isUpload = (kind == TransferKind::Upload);
    const bool pauseRequest = (action == TransferControlAction::Pause);
    requestTaskControl(taskId, isUpload, pauseRequest);
}

QString TransferService::resolvedTaskId(const TransferRequest &request, TransferKind kind) const
{
    if (!request.taskId.isEmpty()) {
        return request.taskId;
    }

    const QString prefix = (kind == TransferKind::Upload) ? QStringLiteral("upload_")
                                                          : QStringLiteral("download_");
    return prefix + QString::number(QDateTime::currentMSecsSinceEpoch());
}

bool TransferService::validateTransferRequest(const TransferRequest &request, const QString &taskId)
{
    if (TransferRequestPolicy::hasUsableNodeEndpoint(request.nodeId,
                                                     request.nodeIp,
                                                     request.nodePort)) {
        return true;
    }

    emitTaskErrorSignal(taskId, QStringLiteral("节点不存在"));
    emitTaskStatusSignal(taskId, static_cast<int>(NetworkTransferStatus::Failed));
    return false;
}

void TransferService::emitTransferFailure(const QString &taskId, const QString &errorMessage)
{
    emitTaskErrorSignal(taskId, errorMessage);
    emitTaskStatusSignal(taskId, static_cast<int>(NetworkTransferStatus::Failed));
}

bool TransferService::finishCanceledTransfer(const QString &taskId, bool isUpload)
{
    cleanupTaskControlState(taskId, isUpload);
    emitTaskStatusSignal(taskId, static_cast<int>(NetworkTransferStatus::Canceled));
    return false;
}

bool TransferService::finishPausedTransfer(const QString &taskId, bool isUpload)
{
    cleanupTaskControlState(taskId, isUpload);
    emitTaskStatusSignal(taskId, static_cast<int>(NetworkTransferStatus::Paused));
    return false;
}

bool TransferService::handleSocketSendFailure(const QString &taskId, bool isUpload, const QString &errorMessage)
{
    switch (checkTaskControlState(taskId, isUpload)) {
    case TaskControlResult::Canceled:
        return finishCanceledTransfer(taskId, isUpload);
    case TaskControlResult::Paused:
        return finishPausedTransfer(taskId, isUpload);
    case TaskControlResult::Continue:
        cleanupTaskControlState(taskId, isUpload);
        emitTransferFailure(taskId, errorMessage);
        return false;
    }

    cleanupTaskControlState(taskId, isUpload);
    emitTransferFailure(taskId, errorMessage);
    return false;
}

void TransferService::enqueueTransferTask(const TransferRequest &request)
{
    if (m_shutdownRequested.load(std::memory_order_acquire)) {
        return;
    }

    QMutexLocker locker(&m_taskQueueMutex);
    m_taskQueue.enqueue(request);
    tryScheduleTaskProcessingLocked();
}

void TransferService::tryScheduleTaskProcessingLocked()
{
    if (m_shutdownRequested.load(std::memory_order_acquire) || m_taskProcessing) {
        return;
    }

    m_taskProcessing = true;
    QMetaObject::invokeMethod(this, &TransferService::processTaskQueue, Qt::QueuedConnection);
}

void TransferService::executeTransferTask(const TransferRequest &request)
{
    if (m_shutdownRequested.load(std::memory_order_acquire)) {
        return;
    }

    if (request.type == TransferKind::Upload) {
        fileUpload(request);
        return;
    }

    fileDownload(request);
}

/**
 * @brief 排空待执行传输队列，并把每个请求交给线程池并发执行。
 * m_taskProcessing 只表示当前是否已有调度回调，不代表传输任务串行执行。
 */
void TransferService::processTaskQueue()
{
    while (true) {
        TransferRequest request;

        {
            QMutexLocker locker(&m_taskQueueMutex);
            if (m_shutdownRequested.load(std::memory_order_acquire)) {
                m_taskQueue.clear();
                m_taskProcessing = false;
                return;
            }

            if (!m_taskQueue.isEmpty()) {
                request = m_taskQueue.dequeue();
            } else {
                m_taskProcessing = false;
                return;
            }
        }

        // 调度线程只负责投递请求；具体传输可并发运行，完成回调再触发下一轮排队检查。
        QThreadPool::globalInstance()->start([this, request]() {
            executeTransferTask(request);

            QMetaObject::invokeMethod(this, [this]() {
                QMutexLocker locker(&m_taskQueueMutex);
                m_taskProcessing = false;
                if (m_taskQueue.isEmpty()) {
                    return;
                }
                tryScheduleTaskProcessingLocked();
            }, Qt::QueuedConnection);
        });
    }
}

TransferService::TaskControlResult TransferService::checkTaskControlState(const QString &taskId, bool isUpload) const
{
    if (m_shutdownRequested.load(std::memory_order_acquire)) {
        return TaskControlResult::Canceled;
    }

    QMutexLocker locker(&m_taskControlMutex);

    const auto direction = isUpload ? TransferControlState::Direction::Upload
                                    : TransferControlState::Direction::Download;
    switch (m_transferControlState.check(direction,
                                         taskId,
                                         m_shutdownRequested.load(std::memory_order_acquire))) {
    case TransferControlState::Result::Continue:
        return TaskControlResult::Continue;
    case TransferControlState::Result::Paused:
        return TaskControlResult::Paused;
    case TransferControlState::Result::Canceled:
        return TaskControlResult::Canceled;
    }

    return TaskControlResult::Canceled;
}

void TransferService::cleanupTaskControlState(const QString &taskId, bool isUpload)
{
    QMutexLocker locker(&m_taskControlMutex);
    m_transferControlState.cleanup(isUpload ? TransferControlState::Direction::Upload
                                            : TransferControlState::Direction::Download,
                                   taskId);
}

void TransferService::requestTaskControl(const QString &taskId, bool isUpload, bool pauseRequest)
{
    QMutexLocker locker(&m_taskControlMutex);
    const auto direction = isUpload ? TransferControlState::Direction::Upload
                                    : TransferControlState::Direction::Download;
    const auto action = pauseRequest ? TransferControlState::Action::Pause
                                     : TransferControlState::Action::Cancel;
    const int socket = m_transferControlState.request(direction, taskId, action);
    if (socket >= 0) {
        TransferProtocolClient::closeSocket(socket);
    }
}

void TransferService::pauseDownload(const QString &taskId)
{
    controlTransfer(taskId, TransferKind::Download, TransferControlAction::Pause);
}

void TransferService::cancelDownload(const QString &taskId)
{
    controlTransfer(taskId, TransferKind::Download, TransferControlAction::Cancel);
}

void TransferService::pauseUpload(const QString &taskId)
{
    controlTransfer(taskId, TransferKind::Upload, TransferControlAction::Pause);
}

void TransferService::cancelUpload(const QString &taskId)
{
    controlTransfer(taskId, TransferKind::Upload, TransferControlAction::Cancel);
}

void TransferService::emitTaskProgressSignal(const QString &taskId, int progress,
                                             qint64 transferred, qint64 total, double speed)
{
    QMetaObject::invokeMethod(this, [=]() {
        emit taskProgressChanged(taskId, progress, transferred, total, speed);
    }, Qt::QueuedConnection);
}

void TransferService::emitTaskStatusSignal(const QString &taskId, int status)
{
    QMetaObject::invokeMethod(this, [=]() {
        emit taskStatusChanged(taskId, status);
    }, Qt::QueuedConnection);
}

void TransferService::emitTaskErrorSignal(const QString &taskId, const QString &errorMessage)
{
    QMetaObject::invokeMethod(this, [=]() {
        emit taskError(taskId, errorMessage);
    }, Qt::QueuedConnection);
}
