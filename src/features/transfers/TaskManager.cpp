/**
 * @file TaskManager.cpp
 * @brief 任务内存仓库、状态迁移和传输请求编排实现。
 *
 * TaskManager 是 Feature 与传输 Gateway 之间的状态边界：它将页面动作
 * 转换为 DTO/控制意图，也把异步回流归并成统一快照和信号。任务对象本身
 * 不创建 socket；所有网络结果都按 taskId 回流到这里再归并。
 */
#include "TaskManager.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>

TaskManager::TaskManager(TaskTransferGateway *transferGateway, QObject *parent)
    : QObject(parent), m_transferGateway(transferGateway), m_taskIdCounter(0)
{
    if (m_transferGateway) {
        connect(m_transferGateway, &TaskTransferGateway::taskProgressChanged,
                this, &TaskManager::updateTaskProgress);
        connect(m_transferGateway, &TaskTransferGateway::taskStatusChanged,
                this, &TaskManager::updateTaskStatus);
        connect(m_transferGateway, &TaskTransferGateway::taskError,
                this, &TaskManager::taskError);
    }
}

TaskManager::~TaskManager() = default;

QString TaskManager::addDownloadTask(const QString &fileName, const QString &savePath,
                                     const QString &nodeId, qint64 fileSize)
{
    QString taskId;
    {
        QMutexLocker locker(&m_taskMutex);

        taskId = QString("download_%1").arg(++m_taskIdCounter);

        DownloadTask task;
        task.taskId = taskId;
        task.fileName = fileName;
        task.savePath = savePath;
        task.nodeId = nodeId;
        task.fileSize = fileSize;
        task.progress = 0;
        task.status = static_cast<int>(TaskStatus::Waiting);
        task.speed = 0.0;
        task.transferredBytes = 0;

        m_downloadTasks.insert(taskId, task);
    }

    // 下载任务创建后立即通知 Waiting，保证列表项与任务仓库拥有相同初始状态。
    emit taskStatusChanged(taskId, static_cast<int>(TaskStatus::Waiting));
    return taskId;
}

QString TaskManager::addUploadTask(const QString &localPath, const QString &nodeId)
{
    QMutexLocker locker(&m_taskMutex);

    const QString taskId = QString("upload_%1").arg(++m_taskIdCounter);
    const QFileInfo fileInfo(localPath);

    UploadTask task;
    task.taskId = taskId;
    task.fileName = fileInfo.fileName();
    task.localPath = localPath;
    task.nodeId = nodeId;
    task.fileSize = fileInfo.size();
    task.progress = 0;
    task.status = static_cast<int>(TaskStatus::Waiting);
    task.speed = 0.0;
    task.transferredBytes = 0;

    m_uploadTasks.insert(taskId, task);
    return taskId;
}

void TaskManager::removeTask(const QString &taskId)
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        if (m_transferGateway) {
            m_transferGateway->controlTransfer(taskId,
                                                TaskTransferGateway::TransferType::Download,
                                                TaskTransferGateway::TransferControlAction::Cancel);
        }
        m_downloadTasks.remove(taskId);
        return;
    }

    if (m_uploadTasks.contains(taskId)) {
        if (m_transferGateway) {
            m_transferGateway->controlTransfer(taskId,
                                                TaskTransferGateway::TransferType::Upload,
                                                TaskTransferGateway::TransferControlAction::Cancel);
        }
        m_uploadTasks.remove(taskId);
    }
}

void TaskManager::pauseTask(const QString &taskId)
{
    bool changed = false;

    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        // 暂停前先把本地已落盘进度补齐，恢复时继续沿当前偏移发起。
        syncDownloadTransferredBytesLocked(m_downloadTasks[taskId]);
        if (m_transferGateway) {
            m_transferGateway->controlTransfer(taskId,
                                                TaskTransferGateway::TransferType::Download,
                                                TaskTransferGateway::TransferControlAction::Pause);
        }
        m_downloadTasks[taskId].status = static_cast<int>(TaskStatus::Paused);
        changed = true;
    } else if (m_uploadTasks.contains(taskId)) {
        syncUploadTransferredBytesLocked(m_uploadTasks[taskId]);
        if (m_transferGateway) {
            m_transferGateway->controlTransfer(taskId,
                                                TaskTransferGateway::TransferType::Upload,
                                                TaskTransferGateway::TransferControlAction::Pause);
        }
        m_uploadTasks[taskId].status = static_cast<int>(TaskStatus::Paused);
        changed = true;
    }
    locker.unlock();

    if (changed) {
        emit taskStatusChanged(taskId, static_cast<int>(TaskStatus::Paused));
    }
}

void TaskManager::cancelTask(const QString &taskId)
{
    bool changed = false;

    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        if (m_transferGateway) {
            m_transferGateway->controlTransfer(taskId,
                                                TaskTransferGateway::TransferType::Download,
                                                TaskTransferGateway::TransferControlAction::Cancel);
        }
        m_downloadTasks[taskId].status = static_cast<int>(TaskStatus::Canceled);
        changed = true;
    } else if (m_uploadTasks.contains(taskId)) {
        if (m_transferGateway) {
            m_transferGateway->controlTransfer(taskId,
                                                TaskTransferGateway::TransferType::Upload,
                                                TaskTransferGateway::TransferControlAction::Cancel);
        }
        m_uploadTasks[taskId].status = static_cast<int>(TaskStatus::Canceled);
        changed = true;
    }
    locker.unlock();

    if (changed) {
        // 取消动作由任务层先给出终态，避免页面还要等底层回流才能更新。
        emit taskStatusChanged(taskId, static_cast<int>(TaskStatus::Canceled));
    }
}

void TaskManager::resumeTask(const QString &taskId)
{
    startTask(taskId);
}

bool TaskManager::startTask(const QString &taskId)
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        return startDownloadTaskLocked(m_downloadTasks[taskId]);
    }

    if (m_uploadTasks.contains(taskId)) {
        return startUploadTaskLocked(m_uploadTasks[taskId]);
    }

    return false;
}

bool TaskManager::getDownloadTask(const QString &taskId, DownloadTask &task) const
{
    QMutexLocker locker(&m_taskMutex);
    if (!m_downloadTasks.contains(taskId)) {
        return false;
    }

    task = m_downloadTasks.value(taskId);
    return true;
}

bool TaskManager::getUploadTask(const QString &taskId, UploadTask &task) const
{
    QMutexLocker locker(&m_taskMutex);
    if (!m_uploadTasks.contains(taskId)) {
        return false;
    }

    task = m_uploadTasks.value(taskId);
    return true;
}

bool TaskManager::getTaskSnapshot(const QString &taskId, TaskSnapshot &task) const
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        const DownloadTask &downloadTask = m_downloadTasks.value(taskId);
        task.taskId = downloadTask.taskId;
        task.fileName = downloadTask.fileName;
        task.nodeId = downloadTask.nodeId;
        task.primaryPath = downloadTask.savePath;
        task.fileSize = downloadTask.fileSize;
        task.transferredBytes = downloadTask.transferredBytes;
        task.progress = downloadTask.progress;
        task.status = downloadTask.status;
        task.speed = downloadTask.speed;
        task.kind = TaskKind::Download;
        return true;
    }

    if (m_uploadTasks.contains(taskId)) {
        const UploadTask &uploadTask = m_uploadTasks.value(taskId);
        task.taskId = uploadTask.taskId;
        task.fileName = uploadTask.fileName;
        task.nodeId = uploadTask.nodeId;
        task.primaryPath = uploadTask.localPath;
        task.fileSize = uploadTask.fileSize;
        task.transferredBytes = uploadTask.transferredBytes;
        task.progress = uploadTask.progress;
        task.status = uploadTask.status;
        task.speed = uploadTask.speed;
        task.kind = TaskKind::Upload;
        return true;
    }

    return false;
}

bool TaskManager::isDownloadTask(const QString &taskId) const
{
    QMutexLocker locker(&m_taskMutex);
    return m_downloadTasks.contains(taskId);
}

bool TaskManager::canStartTask(const QString &taskId) const
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        const DownloadTask &task = m_downloadTasks.value(taskId);
        return task.status == static_cast<int>(TaskStatus::Waiting)
            || task.status == static_cast<int>(TaskStatus::Paused);
    }

    if (m_uploadTasks.contains(taskId)) {
        const UploadTask &task = m_uploadTasks.value(taskId);
        return task.status == static_cast<int>(TaskStatus::Waiting)
            || task.status == static_cast<int>(TaskStatus::Paused);
    }

    return false;
}

bool TaskManager::isTaskRunning(const QString &taskId) const
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        return m_downloadTasks.value(taskId).status == static_cast<int>(TaskStatus::Running);
    }

    if (m_uploadTasks.contains(taskId)) {
        return m_uploadTasks.value(taskId).status == static_cast<int>(TaskStatus::Running);
    }

    return false;
}

bool TaskManager::isTaskCompleted(const QString &taskId) const
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        return m_downloadTasks.value(taskId).status == static_cast<int>(TaskStatus::Completed);
    }

    if (m_uploadTasks.contains(taskId)) {
        return m_uploadTasks.value(taskId).status == static_cast<int>(TaskStatus::Completed);
    }

    return false;
}

QString TaskManager::taskDisplayName(const QString &taskId) const
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        return m_downloadTasks.value(taskId).fileName;
    }

    if (m_uploadTasks.contains(taskId)) {
        return m_uploadTasks.value(taskId).fileName;
    }

    return QString();
}

bool TaskManager::tryStartManagedTask(const QString &taskId, QString *displayName)
{
    if (!canStartTask(taskId)) {
        return false;
    }

    if (displayName) {
        *displayName = taskDisplayName(taskId);
    }

    return startTask(taskId);
}

bool TaskManager::tryPauseManagedTask(const QString &taskId)
{
    if (!isTaskRunning(taskId)) {
        return false;
    }

    pauseTask(taskId);
    return true;
}

QList<QString> TaskManager::getAllTaskIds() const
{
    QMutexLocker locker(&m_taskMutex);
    QList<QString> taskIds = m_downloadTasks.keys();
    taskIds.append(m_uploadTasks.keys());
    return taskIds;
}

/**
 * @brief 根据底层回流进度更新统一任务快照。
 * @param taskId 目标任务 ID。
 * @param progress 底层回流的原始进度值。
 * @param transferredSize 已传输字节数。
 * @param totalSize 总字节数。
 * @param speed 当前传输速度。
 */
void TaskManager::updateTaskProgress(const QString &taskId, int progress,
                                     qint64 transferredSize, qint64 totalSize, double speed)
{
    Q_UNUSED(progress);

    bool found = false;
    int emittedProgress = progressFromBytes(transferredSize, totalSize);

    {
        QMutexLocker locker(&m_taskMutex);

        if (m_downloadTasks.contains(taskId)) {
            DownloadTask &task = m_downloadTasks[taskId];
            task.fileSize = totalSize > 0 ? totalSize : task.fileSize;
            task.transferredBytes = qMax<qint64>(0, transferredSize);
            task.progress = progressFromBytes(task.transferredBytes, task.fileSize);
            task.speed = speed;
            emittedProgress = task.progress;
            found = true;
        } else if (m_uploadTasks.contains(taskId)) {
            UploadTask &task = m_uploadTasks[taskId];
            task.fileSize = totalSize > 0 ? totalSize : task.fileSize;
            task.transferredBytes = qMax<qint64>(0, transferredSize);
            task.progress = progressFromBytes(task.transferredBytes, task.fileSize);
            task.speed = speed;
            emittedProgress = task.progress;
            found = true;
        }
    }

    if (found) {
        // 页面展示统一读取任务层按字节归一化后的进度，不直接信任底层百分比。
        emit taskProgressChanged(taskId, emittedProgress, transferredSize, totalSize, speed);
    }
}

/**
 * @brief 根据底层状态回流更新统一任务终态和恢复偏移。
 * @param taskId 目标任务 ID。
 * @param status 底层回流的状态值。
 */
void TaskManager::updateTaskStatus(const QString &taskId, int status)
{
    // 网络回流入口保留底层状态值；只有暂停和终态需要先同步恢复偏移。
    bool found = false;

    {
        QMutexLocker locker(&m_taskMutex);

        if (m_downloadTasks.contains(taskId)) {
            DownloadTask &task = m_downloadTasks[taskId];
            if (status == static_cast<int>(TaskStatus::Paused) ||
                status == static_cast<int>(TaskStatus::Completed) ||
                status == static_cast<int>(TaskStatus::Failed) ||
                status == static_cast<int>(TaskStatus::Canceled)) {
                // 终态和暂停态都要先同步一次偏移，保证列表展示和下次恢复基于同一份进度。
                syncDownloadTransferredBytesLocked(task);
                if (status == static_cast<int>(TaskStatus::Completed) && task.fileSize > 0) {
                    task.transferredBytes = task.fileSize;
                    task.progress = 100;
                }
            }
            task.status = status;
            found = true;
        } else if (m_uploadTasks.contains(taskId)) {
            UploadTask &task = m_uploadTasks[taskId];
            if (status == static_cast<int>(TaskStatus::Paused) ||
                status == static_cast<int>(TaskStatus::Completed) ||
                status == static_cast<int>(TaskStatus::Failed) ||
                status == static_cast<int>(TaskStatus::Canceled)) {
                syncUploadTransferredBytesLocked(task);
                if (status == static_cast<int>(TaskStatus::Completed) && task.fileSize > 0) {
                    task.transferredBytes = task.fileSize;
                    task.progress = 100;
                }
            }
            task.status = status;
            found = true;
        }
    }

    if (found) {
        emit taskStatusChanged(taskId, status);
    }
}

void TaskManager::resetTaskProgress(const QString &taskId)
{
    QMutexLocker locker(&m_taskMutex);

    if (m_downloadTasks.contains(taskId)) {
        m_downloadTasks[taskId].progress = 0;
        m_downloadTasks[taskId].speed = 0.0;
        m_downloadTasks[taskId].transferredBytes = 0;
        return;
    }

    if (m_uploadTasks.contains(taskId)) {
        m_uploadTasks[taskId].progress = 0;
        m_uploadTasks[taskId].speed = 0.0;
        m_uploadTasks[taskId].transferredBytes = 0;
    }
}

bool TaskManager::tryStartTask(const QString &taskId)
{
    return startTask(taskId);
}

bool TaskManager::isValidTransition(TaskStatus from, TaskStatus to)
{
    switch (from) {
    case TaskStatus::Waiting:
        return to == TaskStatus::Running;
    case TaskStatus::Running:
        return to == TaskStatus::Paused ||
               to == TaskStatus::Completed ||
               to == TaskStatus::Failed ||
               to == TaskStatus::Canceled;
    case TaskStatus::Paused:
        return to == TaskStatus::Running ||
               to == TaskStatus::Failed ||
               to == TaskStatus::Canceled;
    case TaskStatus::Completed:
        return false;
    case TaskStatus::Failed:
        return false;
    case TaskStatus::Canceled:
        return false;
    default:
        return false;
    }
}

/**
 * @brief 校验并原子化更新任务状态。
 * @param taskId 目标任务 ID。
 * @param newStatus 待写入的新状态值。
 * @return 更新是否成功。
 */
bool TaskManager::updateTaskStatusAtomically(const QString &taskId, int newStatus)
{
    const TaskStatus toStatus = static_cast<TaskStatus>(newStatus);

    {
        QMutexLocker locker(&m_taskMutex);

        if (m_downloadTasks.contains(taskId)) {
            const TaskStatus fromStatus = static_cast<TaskStatus>(m_downloadTasks[taskId].status);
            if (!isValidTransition(fromStatus, toStatus)) {
                qWarning() << "[updateTaskStatusAtomically] Invalid transition from"
                           << static_cast<int>(fromStatus) << "to" << newStatus
                           << "for task" << taskId;
                return false;
            }
            m_downloadTasks[taskId].status = newStatus;
        } else if (m_uploadTasks.contains(taskId)) {
            const TaskStatus fromStatus = static_cast<TaskStatus>(m_uploadTasks[taskId].status);
            if (!isValidTransition(fromStatus, toStatus)) {
                qWarning() << "[updateTaskStatusAtomically] Invalid transition from"
                           << static_cast<int>(fromStatus) << "to" << newStatus
                           << "for task" << taskId;
                return false;
            }
            m_uploadTasks[taskId].status = newStatus;
        } else {
            return false;
        }
    }

    emit taskStatusChanged(taskId, newStatus);
    return true;
}

bool TaskManager::startDownloadTaskLocked(DownloadTask &task)
{
    const TaskStatus currentStatus = static_cast<TaskStatus>(task.status);
    if (!isValidTransition(currentStatus, TaskStatus::Running)) {
        return false;
    }

    // 启动入口统一走任务层，底层只接收已经补齐偏移的传输请求。
    const qint64 resumeOffset = syncDownloadTransferredBytesLocked(task);
    task.status = static_cast<int>(TaskStatus::Running);
    task.speed = 0.0;
    if (m_transferGateway) {
        m_transferGateway->startTransfer(buildDownloadTransferRequest(task, resumeOffset));
    }
    return true;
}

bool TaskManager::startUploadTaskLocked(UploadTask &task)
{
    const TaskStatus currentStatus = static_cast<TaskStatus>(task.status);
    if (!isValidTransition(currentStatus, TaskStatus::Running)) {
        return false;
    }

    // 上传和下载都在这里收成统一启动口径，页面层不再区分两套底层签名。
    const qint64 resumeOffset = syncUploadTransferredBytesLocked(task);
    task.status = static_cast<int>(TaskStatus::Running);
    task.speed = 0.0;
    if (m_transferGateway) {
        m_transferGateway->startTransfer(buildUploadTransferRequest(task, resumeOffset));
    }
    return true;
}

TaskTransferGateway::TransferRequest TaskManager::buildDownloadTransferRequest(const DownloadTask &task, qint64 startOffset) const
{
    TaskTransferGateway::TransferRequest request;
    request.type = TaskTransferGateway::TransferType::Download;
    request.fileName = task.fileName;
    request.savePath = task.savePath;
    request.nodeId = task.nodeId;
    request.taskId = task.taskId;
    request.threadCount = 1;
    request.startOffset = startOffset;
    return request;
}

TaskTransferGateway::TransferRequest TaskManager::buildUploadTransferRequest(const UploadTask &task, qint64 startOffset) const
{
    TaskTransferGateway::TransferRequest request;
    request.type = TaskTransferGateway::TransferType::Upload;
    request.filePath = task.localPath;
    request.nodeId = task.nodeId;
    request.taskId = task.taskId;
    request.startOffset = startOffset;
    return request;
}

/**
 * @brief 同步本地下载文件大小，并回写任务恢复偏移。
 * @param task 待同步的下载任务。
 * @return 当前可恢复的下载偏移。
 */
qint64 TaskManager::syncDownloadTransferredBytesLocked(DownloadTask &task)
{
    const QFileInfo fileInfo(task.savePath);
    const qint64 actualBytes = fileInfo.exists() ? fileInfo.size() : 0;
    task.transferredBytes = qMax<qint64>(task.transferredBytes, actualBytes);

    if (task.fileSize > 0) {
        task.transferredBytes = qMin(task.transferredBytes, task.fileSize);
    }

    task.progress = progressFromBytes(task.transferredBytes, task.fileSize);
    return task.transferredBytes;
}

/**
 * @brief 同步本地上传文件信息，并回写任务恢复偏移。
 * @param task 待同步的上传任务。
 * @return 当前可恢复的上传偏移。
 */
qint64 TaskManager::syncUploadTransferredBytesLocked(UploadTask &task)
{
    if (task.fileSize <= 0) {
        const QFileInfo fileInfo(task.localPath);
        task.fileSize = fileInfo.exists() ? fileInfo.size() : 0;
    }

    if (task.fileSize > 0) {
        task.transferredBytes = qMin(task.transferredBytes, task.fileSize);
    } else {
        task.transferredBytes = qMax<qint64>(0, task.transferredBytes);
    }

    task.progress = progressFromBytes(task.transferredBytes, task.fileSize);
    return task.transferredBytes;
}

int TaskManager::progressFromBytes(qint64 transferredBytes, qint64 totalBytes) const
{
    if (totalBytes <= 0) {
        return 0;
    }

    const qint64 normalized = qBound<qint64>(0, transferredBytes, totalBytes);
    return static_cast<int>((normalized * 100) / totalBytes);
}
