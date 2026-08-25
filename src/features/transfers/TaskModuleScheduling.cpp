/*
 * TaskModule 批量调度实现：负责批量启动、暂停、删除和延迟排队。
 * 任务状态展示与终态迁移仍由 TaskModule.cpp 负责。
 */
#include "TaskModule.h"

#include "TaskListController.h"
#include "TaskManager.h"

#include <QListWidget>
#include <QTimer>

/**
 * @brief 批量启动当前选中的任务，并按任务类型做轻量分批。
 * @param taskIds 当前选中的任务 ID 集合。
 * @param isDownload 当前这一轮是否处理下载任务。
 * @param concurrentCount 当前已启动数量计数。
 * @param maxConcurrent 当前批次允许的最大并发数。
 */
void TaskModule::startPendingTasks(const QList<QString> &taskIds,
                                   bool isDownload,
                                   int &concurrentCount,
                                   int maxConcurrent)
{
    // 批量启动继续做轻量分批，先保证现有任务链稳定，不把并发控制压回页面层。
    for (const QString &taskId : taskIds) {
        if (isDownload) {
            startQueuedDownloadTask(taskId, 500 * concurrentCount, concurrentCount, maxConcurrent);
        } else {
            startQueuedUploadTask(taskId, 500 * concurrentCount, concurrentCount, maxConcurrent);
        }
    }
}

void TaskModule::pauseTasksByType(const QList<QString> &taskIds,
                                  bool isDownload,
                                  int &pausedCount,
                                  int &alreadyDoneCount)
{
    if (!m_taskManager) {
        return;
    }

    for (const QString &taskId : taskIds) {
        if (m_taskManager->isDownloadTask(taskId) != isDownload) {
            continue;
        }

        if (m_taskManager->tryPauseManagedTask(taskId)) {
            refreshTaskDisplay(taskId);
            pausedCount++;
        } else if (m_taskManager->isTaskCompleted(taskId)) {
            alreadyDoneCount++;
        }
    }
}

int TaskModule::removeCheckedTaskItems(QListWidget *list, bool removeFromManager)
{
    if (!m_taskListController) {
        return 0;
    }

    const QList<QString> selectedTaskIds = m_taskListController->checkedTaskIds(list);
    for (const QString &taskId : selectedTaskIds) {
        if (removeFromManager && m_taskManager) {
            m_taskManager->removeTask(taskId);
        }
        m_taskListController->removeTaskById(list, taskId);
    }

    m_taskListController->updateTaskSelectAllState(list);
    return selectedTaskIds.size();
}

/**
 * @brief 延迟启动排队中的下载任务。
 * @param taskId 目标任务 ID。
 * @param delayMs 延迟毫秒数。
 * @param concurrentCount 当前已启动数量计数。
 * @param maxConcurrent 当前批次允许的最大并发数。
 */
void TaskModule::startQueuedDownloadTask(const QString &taskId,
                                         int delayMs,
                                         int &concurrentCount,
                                         int maxConcurrent)
{
    if (!m_taskManager || !m_taskManager->isDownloadTask(taskId) || !m_taskManager->canStartTask(taskId)) {
        return;
    }

    if (concurrentCount < maxConcurrent) {
        QString displayName;
        if (!m_taskManager->tryStartManagedTask(taskId, &displayName)) {
            return;
        }
        concurrentCount++;
        refreshTaskDisplay(taskId);
        return;
    }

    const QString delayedTaskId = taskId;
    QTimer::singleShot(delayMs, this, [this, delayedTaskId]() {
        QString displayName;
        if (m_taskManager
            && m_taskManager->isDownloadTask(delayedTaskId)
            && m_taskManager->canStartTask(delayedTaskId)
            && m_taskManager->tryStartManagedTask(delayedTaskId, &displayName)) {
            refreshTaskDisplay(delayedTaskId);
        }
    });
}

/**
 * @brief 延迟启动排队中的上传任务。
 * @param taskId 目标任务 ID。
 * @param delayMs 延迟毫秒数。
 * @param concurrentCount 当前已启动数量计数。
 * @param maxConcurrent 当前批次允许的最大并发数。
 */
void TaskModule::startQueuedUploadTask(const QString &taskId,
                                       int delayMs,
                                       int &concurrentCount,
                                       int maxConcurrent)
{
    if (!m_taskManager || m_taskManager->isDownloadTask(taskId) || !m_taskManager->canStartTask(taskId)) {
        return;
    }

    if (concurrentCount < maxConcurrent) {
        QString displayName;
        if (!m_taskManager->tryStartManagedTask(taskId, &displayName)) {
            return;
        }
        concurrentCount++;
        refreshTaskDisplay(taskId);
        return;
    }

    const QString delayedTaskId = taskId;
    QTimer::singleShot(delayMs, this, [this, delayedTaskId]() {
        QString displayName;
        if (m_taskManager
            && !m_taskManager->isDownloadTask(delayedTaskId)
            && m_taskManager->canStartTask(delayedTaskId)
            && m_taskManager->tryStartManagedTask(delayedTaskId, &displayName)) {
            refreshTaskDisplay(delayedTaskId);
        }
    });
}
