/**
 * @file TaskListController.cpp
 * @brief 任务列表控件操作和行迁移实现。
 *
 * 本文件只处理列表容器和 TaskItem 的组合，不创建或删除 TaskManager
 * 任务。删除/控制请求通过 taskId 信号回到任务模块，避免列表层跨越业务边界。
 */

#include "TaskListController.h"

#include "TaskItem.h"

#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>

TaskListController::TaskListController(QListWidget *activeList,
                                       QListWidget *finishedList,
                                       QCheckBox *activeSelectAll,
                                       QCheckBox *finishedSelectAll,
                                       QObject *parent)
    : QObject(parent)
    , m_activeList(activeList)
    , m_finishedList(finishedList)
    , m_activeSelectAll(activeSelectAll)
    , m_finishedSelectAll(finishedSelectAll)
{
    // 任务项增删和全选联动统一留在列表层，避免主页和任务模块重复操作控件。
}

void TaskListController::bindTaskItem(TaskItem *taskItem, QListWidget *list, const QString &taskId)
{
    if (!taskItem || !list) {
        return;
    }

    connect(taskItem, &TaskItem::deleteTask, this, [this, taskId]() {
        emit taskDeleteRequested(taskId);
    });
    connect(taskItem, &TaskItem::taskControlRequested, this, [this, taskId]() {
        emit taskControlRequested(taskId);
    });
    connect(taskItem, &TaskItem::checkStateChanged, this, [this, list](bool) {
        updateTaskSelectAllState(list);
    });
}

/**
 * @brief 创建一条运行中任务项并加入运行中列表。
 * @param taskId 任务 ID。
 * @param fileName 展示文件名。
 * @return 新建出的任务项；列表不存在时返回空指针。
 */
TaskItem *TaskListController::appendActiveTaskItem(const QString &taskId, const QString &fileName)
{
    if (!m_activeList) {
        return nullptr;
    }

    QListWidgetItem *item = new QListWidgetItem(m_activeList);
    TaskItem *taskItem = new TaskItem(m_activeList);
    item->setSizeHint(taskItem->sizeHint());

    taskItem->setFileName(fileName);
    taskItem->setTaskValue(0);
    taskItem->setTaskStatus(QStringLiteral("等待中"));
    taskItem->setSpeedInfo(QString());

    item->setData(Qt::UserRole, taskId);
    m_activeList->addItem(item);
    m_activeList->setItemWidget(item, taskItem);
    bindTaskItem(taskItem, m_activeList, taskId);
    return taskItem;
}
// 创建一条选中的运行中任务项并加入运行中列表。
// @param taskId 任务 ID。
// @param fileName 展示文件名。
// @return 新建出的任务项；列表不存在时返回空指针。
TaskItem *TaskListController::appendCheckedActiveTaskItem(const QString &taskId,
                                                          const QString &fileName)
{
    TaskItem *taskItem = appendActiveTaskItem(taskId, fileName);
    if (taskItem) {
        taskItem->setChecked(true);
    }
    if (m_activeList) {
        updateTaskSelectAllState(m_activeList);
    }
    return taskItem;
}

TaskItem *TaskListController::appendFinishedTaskItem(const QString &taskId,
                                                     const QString &fileName,
                                                     int progress,
                                                     const QString &statusText,
                                                     const QString &speedInfo)
{
    if (!m_finishedList) {
        return nullptr;
    }

    QListWidgetItem *item = new QListWidgetItem(m_finishedList);
    TaskItem *taskItem = new TaskItem(m_finishedList);
    item->setSizeHint(taskItem->sizeHint());

    taskItem->setFileName(fileName);
    taskItem->setTaskValue(progress);
    taskItem->setTaskStatus(statusText);
    taskItem->setSpeedInfo(speedInfo);

    item->setData(Qt::UserRole, taskId);
    m_finishedList->addItem(item);
    m_finishedList->setItemWidget(item, taskItem);
    bindTaskItem(taskItem, m_finishedList, taskId);
    return taskItem;
}

QList<QListWidgetItem*> TaskListController::checkedTaskItems(QListWidget *list) const
{
    QList<QListWidgetItem*> selectedItems;
    if (!list) {
        return selectedItems;
    }

    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem *item = list->item(i);
        TaskItem *taskItem = item ? qobject_cast<TaskItem*>(list->itemWidget(item)) : nullptr;
        if (taskItem && taskItem->isChecked()) {
            selectedItems.append(item);
        }
    }

    return selectedItems;
}

QList<QString> TaskListController::checkedTaskIds(QListWidget *list) const
{
    QList<QString> taskIds;
    const QList<QListWidgetItem*> selectedItems = checkedTaskItems(list);
    for (QListWidgetItem *item : selectedItems) {
        if (!item) {
            continue;
        }
        const QString taskId = item->data(Qt::UserRole).toString();
        if (!taskId.isEmpty()) {
            taskIds.append(taskId);
        }
    }
    return taskIds;
}

bool TaskListController::taskExistsInList(QListWidget *list, const QString &taskId) const
{
    return findTaskListItem(list, taskId) != nullptr;
}

int TaskListController::removeCheckedTaskItems(QListWidget *list)
{
    QList<QListWidgetItem*> selectedItems = checkedTaskItems(list);
    for (QListWidgetItem *item : selectedItems) {
        const int row = list ? list->row(item) : -1;
        if (row >= 0) {
            delete list->takeItem(row);
        }
    }

    updateTaskSelectAllState(list);
    return selectedItems.size();
}
// 获取当前勾选的任务 ID 列表。
QList<QString> TaskListController::selectedTaskIds() const
{
    QList<QString> selectedIds;
    // 先获取运行中列表的勾选任务 ID。
    if (!m_activeList) {
        return selectedIds;
    }

    for (int i = 0; i < m_activeList->count(); ++i) {
        QListWidgetItem *item = m_activeList->item(i);
        TaskItem *taskItem = item ? qobject_cast<TaskItem*>(m_activeList->itemWidget(item)) : nullptr;
        if (taskItem && taskItem->isChecked()) {
            const QString taskId = item->data(Qt::UserRole).toString();
            if (!taskId.isEmpty()) {
                selectedIds.append(taskId);
            }
        }
    }

    return selectedIds;
}

void TaskListController::setTaskItemsChecked(QListWidget *list, bool checked)
{
    if (!list) {
        return;
    }

    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem *item = list->item(i);
        TaskItem *taskItem = item ? qobject_cast<TaskItem*>(list->itemWidget(item)) : nullptr;
        if (taskItem) {
            taskItem->setChecked(checked);
        }
    }
}

void TaskListController::updateTaskSelectAllState(QListWidget *list) const
{
    if (!list) {
        return;
    }

    QCheckBox *selectAllCheckBox = nullptr;
    if (list == m_activeList) {
        selectAllCheckBox = m_activeSelectAll;
    } else if (list == m_finishedList) {
        selectAllCheckBox = m_finishedSelectAll;
    }

    if (!selectAllCheckBox) {
        return;
    }

    const bool hasItems = (list->count() > 0);
    const bool allChecked = hasItems && checkedTaskItems(list).size() == list->count();
    selectAllCheckBox->blockSignals(true);
    selectAllCheckBox->setCheckState(allChecked ? Qt::Checked : Qt::Unchecked);
    selectAllCheckBox->blockSignals(false);
}

void TaskListController::removeTaskById(QListWidget *list, const QString &taskId)
{
    if (!list) {
        return;
    }

    QListWidgetItem *item = findTaskListItem(list, taskId);
    if (!item) {
        return;
    }

    const int row = list->row(item);
    if (row >= 0) {
        delete list->takeItem(row);
    }
    updateTaskSelectAllState(list);
}

TaskItem *TaskListController::findTaskItem(QListWidget *list, const QString &taskId) const
{
    QListWidgetItem *item = findTaskListItem(list, taskId);
    return (item && list) ? qobject_cast<TaskItem*>(list->itemWidget(item)) : nullptr;
}

QListWidgetItem *TaskListController::findTaskListItem(QListWidget *list, const QString &taskId) const
{
    if (!list) {
        return nullptr;
    }

    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem *item = list->item(i);
        if (item && item->data(Qt::UserRole).toString() == taskId) {
            return item;
        }
    }

    return nullptr;
}

bool TaskListController::hasActiveTasks() const
{
    return m_activeList && m_activeList->count() > 0;
}

void TaskListController::updateTaskDisplay(const QString &taskId, int progress, const QString &statusText, const QString &speedInfo)
{
    TaskItem *taskItem = findTaskItem(m_activeList, taskId);
    if (!taskItem) {
        return;
    }

    taskItem->setTaskValue(progress);
    taskItem->setTaskStatus(statusText);
    if (!speedInfo.isNull()) {
        taskItem->setSpeedInfo(speedInfo);
    }
}

/**
 * @brief 将运行中任务迁到已完成列表。
 * @param taskId 任务 ID。
 * @param fileName 展示文件名。
 * @param progress 迁移时展示的最终进度。
 * @param statusText 已完成列表中的终态文案。
 * @param speedInfo 已完成列表中的大小或速度文案。
 */
void TaskListController::moveActiveTaskToFinished(const QString &taskId,
                                                  const QString &fileName,
                                                  int progress,
                                                  const QString &statusText,
                                                  const QString &speedInfo)
{
    appendFinishedTaskItem(taskId, fileName, progress, statusText, speedInfo);
    removeTaskById(m_activeList, taskId);
    if (m_finishedList) {
        updateTaskSelectAllState(m_finishedList);
        m_finishedList->update();
    }
    if (m_activeList) {
        updateTaskSelectAllState(m_activeList);
        m_activeList->update();
    }
}
