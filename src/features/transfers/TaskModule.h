#ifndef TASKMODULE_H
#define TASKMODULE_H

#include <QObject>
#include <QString>

class QListWidget;
class TaskNodeNameGateway;
class TaskListController;
class TaskManager;

// 任务模块本体：同时协调任务语义和任务列表控件；TaskManager 负责状态事实，
// TaskListController 负责单项列表操作，批量迁移和延迟启动仍由本模块编排。
class TaskModule : public QObject
{
    Q_OBJECT

public:
    struct ActionResult {
        QString message;
        bool warning = false;
    };

    struct FinishedTaskInfo {
        bool moved = false;
        QString taskId;
        QString fileName;
        QString nodeName;
        QString taskType;
        QString speedInfo;
        bool isDownload = false;
        int status = 0;
    };

    explicit TaskModule(TaskManager *taskManager,
                        TaskListController *taskListController,
                        TaskNodeNameGateway *nodeNameGateway,
                        QListWidget *activeList,
                        QListWidget *finishedList,
                        QObject *parent = nullptr);

    /**
     * @brief 把新建任务加入运行中列表，并接上统一启动链。
     * @param taskId 新建任务 ID。
     * @param fileName 页面展示文件名。
     */
    void appendCreatedTaskAndStart(const QString &taskId,
                                   const QString &fileName);
    void deleteTaskById(const QString &taskId);
    ActionResult toggleTaskById(const QString &taskId);
    ActionResult startSelectedTasks();
    ActionResult pauseSelectedTasks();
    ActionResult cancelSelectedActiveTasks();
    void setActiveTasksChecked(bool checked);
    void setFinishedTasksChecked(bool checked);
    void deleteSelectedFinishedTasks();
    void updateTaskProgressDisplay(const QString &taskId,
                                   qint64 transferredSize,
                                   qint64 totalSize,
                                   double speed);
    void refreshTaskDisplay(const QString &taskId);
    /**
     * @brief 处理任务终态，将运行中任务迁到已完成列表。
     * @param taskId 任务 ID。
     * @param status 当前终态值。
     * @param finishedTaskInfo 返回迁移后的摘要信息。
     * @return 是否成功完成本次迁移。
     */
    bool handleTaskCompletion(const QString &taskId, int status, FinishedTaskInfo &finishedTaskInfo);
    bool hasActiveTasks() const;
    bool taskExistsInFinishedList(const QString &taskId) const;

private:
    // 任务页展示规则由无 QObject 策略负责；本模块只保留快照查询和列表编排。
    QString finishedTaskSpeedInfo(const QString &taskId, bool isSuccess) const;
    bool tryAppendFinishedTaskFromRunningList(const QString &taskId,
                                              int status,
                                              FinishedTaskInfo &finishedTaskInfo);
    void startPendingTasks(const QList<QString> &taskIds,
                           bool isDownload,
                           int &concurrentCount,
                           int maxConcurrent);
    void pauseTasksByType(const QList<QString> &taskIds,
                          bool isDownload,
                          int &pausedCount,
                          int &alreadyDoneCount);
    int removeCheckedTaskItems(QListWidget *list, bool removeFromManager);
    void startQueuedDownloadTask(const QString &taskId,
                                 int delayMs,
                                 int &concurrentCount,
                                 int maxConcurrent);
    void startQueuedUploadTask(const QString &taskId,
                               int delayMs,
                               int &concurrentCount,
                               int maxConcurrent);

    TaskManager *m_taskManager;
    TaskListController *m_taskListController;
    TaskNodeNameGateway *m_nodeNameGateway;
    QListWidget *m_activeList;
    QListWidget *m_finishedList;
};

#endif // TASKMODULE_H
