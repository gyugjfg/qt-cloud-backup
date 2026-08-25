#include "TaskModule.h"

#include "TaskNodeNameGateway.h"
#include "TaskItem.h"
#include "TaskListController.h"
#include "TaskManager.h"
#include "TaskPresentationPolicy.h"

#include <QFileInfo>
#include <QListWidget>
#include <QTimer>

TaskModule::TaskModule(TaskManager *taskManager,
                       TaskListController *taskListController,
                       TaskNodeNameGateway *nodeNameGateway,
                       QListWidget *activeList,
                       QListWidget *finishedList,
                       QObject *parent)
    : QObject(parent)
    , m_taskManager(taskManager)
    , m_taskListController(taskListController)
    , m_nodeNameGateway(nodeNameGateway)
    , m_activeList(activeList)
    , m_finishedList(finishedList)
{
}

void TaskModule::appendCreatedTaskAndStart(const QString &taskId,
                                           const QString &fileName)
{
    if (!m_taskListController || taskId.isEmpty()) {
        return;
    }

    // 新任务先加入运行中列表，再走统一启动链，使页面反馈与任务状态保持同一节奏。
    m_taskListController->appendCheckedActiveTaskItem(taskId,
                                                      fileName);

    QString displayName;
    if (m_taskManager && m_taskManager->tryStartManagedTask(taskId, &displayName)) {
        refreshTaskDisplay(taskId);
    }
}

void TaskModule::deleteTaskById(const QString &taskId)
{
    if (taskId.isEmpty()) {
        return;
    }

    if (m_taskManager) {
        m_taskManager->removeTask(taskId);
    }

    if (m_taskListController) {
        m_taskListController->removeTaskById(m_activeList, taskId);
        m_taskListController->removeTaskById(m_finishedList, taskId);
    }
}

TaskModule::ActionResult TaskModule::toggleTaskById(const QString &taskId)
{
    ActionResult result;
    if (!m_taskManager || taskId.isEmpty()) {
        return result;
    }

    TaskManager::TaskSnapshot snapshot;
    if (!m_taskManager->getTaskSnapshot(taskId, snapshot)) {
        result.message = QStringLiteral("任务已不存在或状态已变化");
        result.warning = true;
        return result;
    }

    const auto status = static_cast<TaskManager::TaskStatus>(snapshot.status);
    if (status == TaskManager::TaskStatus::Running) {
        if (!m_taskManager->tryPauseManagedTask(taskId)) {
            result.message = QStringLiteral("当前任务无法暂停");
            result.warning = true;
        }
    } else if (status == TaskManager::TaskStatus::Waiting || status == TaskManager::TaskStatus::Paused) {
        QString displayName;
        if (!m_taskManager->tryStartManagedTask(taskId, &displayName)) {
            result.message = QStringLiteral("当前任务无法继续");
            result.warning = true;
        }
    } else {
        result.message = QStringLiteral("任务已结束，无法继续操作");
        result.warning = true;
    }

    refreshTaskDisplay(taskId);
    return result;
}

TaskModule::ActionResult TaskModule::startSelectedTasks()
{
    ActionResult result;
    if (!m_taskListController) {
        return result;
    }

    int concurrentCount = 0;
    const int maxConcurrent = 5;
    const QList<QString> selectedTaskIds = m_taskListController->selectedTaskIds();

    if (selectedTaskIds.isEmpty()) {
        result.message = QStringLiteral("请先选择要启动的任务");
        result.warning = true;
        return result;
    }

    // 上传和下载继续共用一套批量启动入口，只在内部按任务类型分发。
    startPendingTasks(selectedTaskIds, true, concurrentCount, maxConcurrent);
    startPendingTasks(selectedTaskIds, false, concurrentCount, maxConcurrent);
    return result;
}

TaskModule::ActionResult TaskModule::pauseSelectedTasks()
{
    ActionResult result;
    if (!m_taskListController) {
        return result;
    }

    int pausedCount = 0;
    int alreadyDoneCount = 0;
    const QList<QString> selectedTaskIds = m_taskListController->selectedTaskIds();

    if (selectedTaskIds.isEmpty()) {
        result.message = QStringLiteral("请先选择要暂停的任务");
        result.warning = true;
        return result;
    }

    pauseTasksByType(selectedTaskIds, true, pausedCount, alreadyDoneCount);
    pauseTasksByType(selectedTaskIds, false, pausedCount, alreadyDoneCount);

    if (pausedCount > 0 && alreadyDoneCount > 0) {
        result.message = QStringLiteral("已暂停 %1 个任务\n%2 个任务已经完成，无法暂停").arg(pausedCount).arg(alreadyDoneCount);
    } else if (pausedCount > 0) {
        result.message = QStringLiteral("已暂停选中的任务");
    } else if (alreadyDoneCount > 0) {
        result.message = QStringLiteral("暂停失败：选中的任务已完成");
    } else {
        result.message = QStringLiteral("没有需要暂停的任务");
    }
    return result;
}

TaskModule::ActionResult TaskModule::cancelSelectedActiveTasks()
{
    ActionResult result;
    const int removedCount = removeCheckedTaskItems(m_activeList, true);
    if (removedCount > 0) {
        result.message = QStringLiteral("已取消 %1 个任务").arg(removedCount);
    } else {
        result.message = QStringLiteral("请先选择要取消的任务");
        result.warning = true;
    }
    return result;
}

void TaskModule::setActiveTasksChecked(bool checked)
{
    if (!m_taskListController) {
        return;
    }

    m_taskListController->setTaskItemsChecked(m_activeList, checked);
    m_taskListController->updateTaskSelectAllState(m_activeList);
}

void TaskModule::setFinishedTasksChecked(bool checked)
{
    if (!m_taskListController) {
        return;
    }

    m_taskListController->setTaskItemsChecked(m_finishedList, checked);
    m_taskListController->updateTaskSelectAllState(m_finishedList);
}

void TaskModule::deleteSelectedFinishedTasks()
{
    removeCheckedTaskItems(m_finishedList, false);
}

void TaskModule::updateTaskProgressDisplay(const QString &taskId,
                                           qint64 transferredSize,
                                           qint64 totalSize,
                                           double speed)
{
    Q_UNUSED(transferredSize);
    if (!m_taskListController || !m_taskManager) {
        return;
    }

    TaskManager::TaskSnapshot taskSnapshot;
    if (!m_taskManager->getTaskSnapshot(taskId, taskSnapshot)) {
        return;
    }

    if (taskSnapshot.status == static_cast<int>(TaskManager::TaskStatus::Paused)) {
        return;
    }

    QString speedStr;
    if (speed > 1024) {
        speedStr = QString::number(speed / 1024.0, 'f', 1) + " MB/s";
    } else {
        speedStr = QString::number(speed, 'f', 1) + " KB/s";
    }

    const qint64 displayTransferredSize = taskSnapshot.transferredBytes;
    const qint64 displayTotalSize = taskSnapshot.fileSize > 0 ? taskSnapshot.fileSize : totalSize;
    const QString sizeStr = TaskPresentationPolicy::formatFileSize(displayTransferredSize)
        + " / " + TaskPresentationPolicy::formatFileSize(displayTotalSize);
    m_taskListController->updateTaskDisplay(taskId,
                                            taskSnapshot.progress,
                                            TaskPresentationPolicy::taskStatusText(
                                                taskSnapshot.status,
                                                taskSnapshot.kind == TaskManager::TaskKind::Download),
                                            speedStr + "  |  " + sizeStr);
}

void TaskModule::refreshTaskDisplay(const QString &taskId)
{
    if (!m_taskListController || !m_taskManager) {
        return;
    }

    TaskManager::TaskSnapshot taskSnapshot;
    if (!m_taskManager->getTaskSnapshot(taskId, taskSnapshot)) {
        return;
    }

    m_taskListController->updateTaskDisplay(taskId,
                                            taskSnapshot.progress,
                                            TaskPresentationPolicy::taskStatusText(
                                                taskSnapshot.status,
                                                taskSnapshot.kind == TaskManager::TaskKind::Download));
}

bool TaskModule::handleTaskCompletion(const QString &taskId, int status, FinishedTaskInfo &finishedTaskInfo)
{
    finishedTaskInfo = FinishedTaskInfo();

    if (taskExistsInFinishedList(taskId)) {
        return false;
    }

    TaskItem *taskItem = m_taskListController ? m_taskListController->findTaskItem(m_activeList, taskId) : nullptr;
    if (!taskItem) {
        return tryAppendFinishedTaskFromRunningList(taskId, status, finishedTaskInfo);
    }

    if (!m_taskManager || !m_taskListController) {
        return false;
    }

    // 任务完成后的运行中 -> 已完成迁移仍留在任务模块，避免主页和列表层各自维护一份终态收尾。
    TaskManager::TaskSnapshot taskSnapshot;
    if (!m_taskManager->getTaskSnapshot(taskId, taskSnapshot)) {
        return false;
    }

    const bool isDownload = (taskSnapshot.kind == TaskManager::TaskKind::Download);
    const QString fileName = taskItem->getFileName();
    const QString nodeName = m_nodeNameGateway ? m_nodeNameGateway->nodeName(taskSnapshot.nodeId) : QString();
    int progress = taskSnapshot.progress;
    qint64 fileSize = taskSnapshot.fileSize;
    qint64 transferredBytes = taskSnapshot.transferredBytes;

    if (fileSize <= 0) {
        const QFileInfo fileInfo(taskSnapshot.primaryPath);
        if (fileInfo.exists()) {
            fileSize = fileInfo.size();
        }
    }

    const bool isSuccess = (status == static_cast<int>(TaskManager::TaskStatus::Completed));
    if (isSuccess) {
        progress = 100;
    }

    QString speedInfo;
    if (isSuccess && fileSize > 0) {
        speedInfo = TaskPresentationPolicy::formatFileSize(fileSize);
    }
    if (speedInfo.isEmpty() && transferredBytes > 0) {
        speedInfo = TaskPresentationPolicy::formatFileSize(transferredBytes);
    }
    if (speedInfo.isEmpty()) {
        speedInfo = isSuccess ? QString("--")
                              : (transferredBytes > 0
                                     ? TaskPresentationPolicy::formatFileSize(transferredBytes)
                                     : TaskPresentationPolicy::failedText());
    }

    // 运行中列表展示信息先就地取出，再迁到已完成列表，避免终态回流后丢 UI 上下文。
    m_taskListController->moveActiveTaskToFinished(taskId,
                                                   fileName,
                                                   progress,
                                                   TaskPresentationPolicy::finishedStatusText(status, isDownload),
                                                   speedInfo);
    m_taskManager->removeTask(taskId);
    m_taskListController->updateTaskSelectAllState(m_activeList);
    m_taskListController->updateTaskSelectAllState(m_finishedList);

    finishedTaskInfo.moved = true;
    finishedTaskInfo.taskId = taskId;
    finishedTaskInfo.fileName = fileName;
    finishedTaskInfo.nodeName = nodeName;
    finishedTaskInfo.taskType = TaskPresentationPolicy::taskTypeText(isDownload);
    finishedTaskInfo.speedInfo = speedInfo;
    finishedTaskInfo.isDownload = isDownload;
    finishedTaskInfo.status = status;
    return true;
}

bool TaskModule::hasActiveTasks() const
{
    return m_taskListController && m_taskListController->hasActiveTasks();
}

bool TaskModule::taskExistsInFinishedList(const QString &taskId) const
{
    return m_taskListController && m_taskListController->taskExistsInList(m_finishedList, taskId);
}

QString TaskModule::finishedTaskSpeedInfo(const QString &taskId, bool isSuccess) const
{
    if (!isSuccess || !m_taskManager) {
        return TaskPresentationPolicy::failedText();
    }

    TaskManager::TaskSnapshot taskSnapshot;
    if (!m_taskManager->getTaskSnapshot(taskId, taskSnapshot)) {
        return QString();
    }

    qint64 fileSize = taskSnapshot.fileSize;
    if (fileSize <= 0) {
        const QFileInfo fileInfo(taskSnapshot.primaryPath);
        if (fileInfo.exists()) {
            fileSize = fileInfo.size();
        }
    }
    if (fileSize <= 0) {
        return QString();
    }

    return TaskPresentationPolicy::formatFileSize(fileSize);
}

bool TaskModule::tryAppendFinishedTaskFromRunningList(const QString &taskId,
                                                      int status,
                                                      FinishedTaskInfo &finishedTaskInfo)
{
    if (!m_taskListController || !m_taskManager) {
        return false;
    }

    TaskItem *taskItem = m_taskListController->findTaskItem(m_activeList, taskId);
    if (!taskItem) {
        m_taskListController->removeTaskById(m_activeList, taskId);
        return false;
    }

    const QString fileName = taskItem->getFileName();
    const bool isDownload = !taskId.startsWith("upload_");
    const bool isSuccess = (status == static_cast<int>(TaskManager::TaskStatus::Completed));
    QString speedInfo = finishedTaskSpeedInfo(taskId, isSuccess);
    if (speedInfo.isEmpty()) {
        speedInfo = TaskPresentationPolicy::sizeFromProgressDisplay(taskItem->getSpeedInfo());
    }
    if (speedInfo.isEmpty()) {
        speedInfo = isSuccess ? QString("--")
                              : (status == static_cast<int>(TaskManager::TaskStatus::Canceled)
                                     ? TaskPresentationPolicy::canceledText()
                                     : TaskPresentationPolicy::failedText());
    }

    m_taskListController->moveActiveTaskToFinished(taskId,
                                                   fileName,
                                                   isSuccess ? 100 : taskItem->getTaskValue(),
                                                   TaskPresentationPolicy::finishedStatusText(status, isDownload),
                                                   speedInfo);
    m_taskManager->removeTask(taskId);
    m_taskListController->updateTaskSelectAllState(m_activeList);

    finishedTaskInfo.moved = true;
    finishedTaskInfo.taskId = taskId;
    finishedTaskInfo.fileName = fileName;
    finishedTaskInfo.taskType = TaskPresentationPolicy::taskTypeText(isDownload);
    finishedTaskInfo.speedInfo = speedInfo;
    finishedTaskInfo.isDownload = isDownload;
    finishedTaskInfo.status = status;
    return true;
}
