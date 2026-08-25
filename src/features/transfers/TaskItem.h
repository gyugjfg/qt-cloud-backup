#ifndef TASKITEM_H
#define TASKITEM_H

/**
 * @file TaskItem.h
 * @brief 任务列表单行控件的展示接口。
 *
 * TaskItem 只操作自身 Designer 控件并发出局部交互信号，不保存任务业务
 * 状态，也不直接调用 TaskManager 或网络服务。控件由 QListWidget 的行
 * 容器拥有，调用方应在 GUI 线程更新展示值。
 */

#include <QWidget>

namespace Ui {
class TaskItem;
}

// 任务列表动态项：负责展示单条任务状态，并把删除/勾选动作转发给列表层。
class TaskItem : public QWidget
{
    Q_OBJECT

public:
    explicit TaskItem(QWidget *parent = nullptr);
    ~TaskItem();

    /** 设置任务行文件名；文本不做路径解析。 */
    void setFileName(QString name);
    /** 设置进度条数值，范围语义沿用 QProgressBar。 */
    void setTaskValue(int value);
    /** 设置状态文案并同步状态样式和开始/暂停按钮可见性。 */
    void setTaskStatus(QString status);
    /** setTaskValue 的语义化别名，供列表层更新进度。 */
    void setProgress(int value);
    /** 设置速度或大小辅助文案。 */
    void setSpeedInfo(QString info);

    /** 返回当前文件名展示文本。 */
    QString getFileName();
    /** 返回当前速度或大小辅助文案。 */
    QString getSpeedInfo();
    /** 返回当前进度条数值。 */
    int getTaskValue();
    /** 返回当前状态展示文案。 */
    QString getTaskStatus();
    /** 返回行内复选框是否选中。 */
    bool isChecked();
    /** 设置行内复选框状态。 */
    void setChecked(bool checked);

private slots:
    void onDeleteButtonClicked();
    void onTaskControlButtonClicked();

private:
    Ui::TaskItem *ui;

signals:
    void deleteTask();
    void taskControlRequested();
    void checkStateChanged(bool checked);
};

#endif // TASKITEM_H
