#ifndef TASKLISTCONTROLLER_H
#define TASKLISTCONTROLLER_H

/**
 * @file TaskListController.h
 * @brief 运行中/已完成任务列表的控件边界。
 *
 * 控制器只负责 QListWidget 行创建、查找、勾选联动和列表迁移；任务状态
 * 决策、网络控制和任务仓库均由 TaskModule/TaskManager 负责。列表和复选框
 * 是组合根注入的借用指针，方法应在 GUI 线程调用。
 */

#include <QObject>
#include <QList>

class QListWidget;
class QListWidgetItem;
class QCheckBox;
class TaskItem;

// 任务页控制层：只负责任务列表 UI，本身不做任务调度和状态决策。
class TaskListController : public QObject
{
    Q_OBJECT

public:
    explicit TaskListController(QListWidget *activeList,
                                QListWidget *finishedList,
                                QCheckBox *activeSelectAll,
                                QCheckBox *finishedSelectAll,
                                QObject *parent = nullptr);

    /** 创建等待中的运行中任务行，并在 UserRole 保存 taskId。 */
    TaskItem *appendActiveTaskItem(const QString &taskId, const QString &fileName);
    /** 创建运行中任务行并立即选中，用于新建任务批次。 */
    TaskItem *appendCheckedActiveTaskItem(const QString &taskId, const QString &fileName);
    /** 创建已完成列表任务行并写入最终展示值。 */
    TaskItem *appendFinishedTaskItem(const QString &taskId,
                                     const QString &fileName,
                                     int progress,
                                     const QString &statusText,
                                     const QString &speedInfo);
    /** 返回指定列表中被选中的行；输入列表为空时返回空集合。 */
    QList<QListWidgetItem*> checkedTaskItems(QListWidget *list) const;
    /** 从指定列表的勾选行提取非空 taskId。 */
    QList<QString> checkedTaskIds(QListWidget *list) const;
    /** 判断 taskId 是否存在于指定列表。 */
    bool taskExistsInList(QListWidget *list, const QString &taskId) const;
    /** 删除指定列表中的全部勾选行并刷新全选状态，返回删除数量。 */
    int removeCheckedTaskItems(QListWidget *list);
    /** 返回运行中列表的勾选任务 ID。 */
    QList<QString> selectedTaskIds() const;
    /** 批量设置指定列表的勾选状态。 */
    void setTaskItemsChecked(QListWidget *list, bool checked);
    /** 按当前勾选情况刷新对应列表的全选复选框。 */
    void updateTaskSelectAllState(QListWidget *list) const;
    /** 按 taskId 删除指定列表的一行；找不到时不产生副作用。 */
    void removeTaskById(QListWidget *list, const QString &taskId);
    /** 查找任务行控件；找不到时返回 nullptr。 */
    TaskItem *findTaskItem(QListWidget *list, const QString &taskId) const;
    /** 查找承载 taskId 的 QListWidgetItem；找不到时返回 nullptr。 */
    QListWidgetItem *findTaskListItem(QListWidget *list, const QString &taskId) const;
    /** 返回运行中列表是否至少存在一条任务。 */
    bool hasActiveTasks() const;
    /** 更新运行中任务行的进度、状态和可选速度文案。 */
    void updateTaskDisplay(const QString &taskId, int progress, const QString &statusText, const QString &speedInfo = QString());
    /** 将运行中任务按给定终态展示值迁移到已完成列表。 */
    void moveActiveTaskToFinished(const QString &taskId,
                                  const QString &fileName,
                                  int progress,
                                  const QString &statusText,
                                  const QString &speedInfo);

signals:
    void taskDeleteRequested(const QString &taskId);
    void taskControlRequested(const QString &taskId);

private:
    void bindTaskItem(TaskItem *taskItem, QListWidget *list, const QString &taskId);
    // 运行中任务列表。
    QListWidget *m_activeList;
    QListWidget *m_finishedList;
    QCheckBox *m_activeSelectAll;
    QCheckBox *m_finishedSelectAll;
};

#endif // TASKLISTCONTROLLER_H
