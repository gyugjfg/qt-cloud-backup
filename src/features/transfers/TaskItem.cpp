/**
 * @file TaskItem.cpp
 * @brief 任务列表单行控件的展示和局部交互实现。
 *
 * 状态文案到样式属性的映射集中在本控件内；删除、暂停/继续和勾选只
 * 通过信号上抛，具体任务操作由 TaskListController/TaskModule 编排。
 */

#include "TaskItem.h"
#include "ui_TaskItem.h"

#include <QIcon>
#include <QStyle>

namespace {
QString utf8Text(const char *text)
{
    return QString::fromUtf8(text);
}

QString waitingText()
{
    return utf8Text("\xE7\xAD\x89\xE5\xBE\x85\xE4\xB8\xAD");
}

QString downloadingText()
{
    return utf8Text("\xE4\xB8\x8B\xE8\xBD\xBD\xE4\xB8\xAD");
}

QString uploadingText()
{
    return utf8Text("\xE4\xB8\x8A\xE4\xBC\xA0\xE4\xB8\xAD");
}

QString pausedText()
{
    return utf8Text("\xE5\xB7\xB2\xE6\x9A\x82\xE5\x81\x9C");
}

QString completedText()
{
    return utf8Text("\xE5\xB7\xB2\xE5\xAE\x8C\xE6\x88\x90");
}

QString downloadedText()
{
    return utf8Text("\xE5\xB7\xB2\xE4\xB8\x8B\xE8\xBD\xBD");
}

QString uploadedText()
{
    return utf8Text("\xE5\xB7\xB2\xE4\xB8\x8A\xE4\xBC\xA0");
}

QString failedText()
{
    return utf8Text("\xE5\xA4\xB1\xE8\xB4\xA5");
}

QString canceledText()
{
    return utf8Text("\xE5\xB7\xB2\xE5\x8F\x96\xE6\xB6\x88");
}
}

TaskItem::TaskItem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TaskItem)
{
    // 任务项本身只负责展示和局部交互，删除动作继续转发给列表层。
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(70);
    setMaximumHeight(70);
    connect(ui->DeleteButton, &QPushButton::clicked, this, &TaskItem::onDeleteButtonClicked);
    connect(ui->TaskControlButton, &QPushButton::clicked, this, &TaskItem::onTaskControlButtonClicked);
    connect(ui->Check, &QCheckBox::toggled, this, &TaskItem::checkStateChanged);
}

TaskItem::~TaskItem()
{
    delete ui;
}

void TaskItem::onDeleteButtonClicked()
{
    emit deleteTask();
}

void TaskItem::onTaskControlButtonClicked()
{
    emit taskControlRequested();
}

void TaskItem::setFileName(QString name)
{
    ui->FileName->setText(name);
}

void TaskItem::setProgress(int value)
{
    ui->progressBar->setValue(value);
}

void TaskItem::setTaskValue(int value)
{
    setProgress(value);
}

void TaskItem::setTaskStatus(QString status)
{
    ui->StatusLabel->setText(status);

    QString state = QStringLiteral("waiting");
    if (status == waitingText()) {
        state = QStringLiteral("waiting");
    } else if (status == downloadingText() || status == uploadingText()) {
        state = QStringLiteral("active");
    } else if (status == pausedText()) {
        state = QStringLiteral("warning");
    } else if (status == completedText() || status == downloadedText() || status == uploadedText()) {
        state = QStringLiteral("success");
    } else if (status == failedText() || status == canceledText()) {
        state = QStringLiteral("error");
    }

    ui->StatusLabel->setProperty("state", state);
    ui->progressBar->setProperty("state", state);

    const bool isRunning = status == downloadingText() || status == uploadingText();
    const bool canStart = status == waitingText() || status == pausedText();
    ui->TaskControlButton->setVisible(isRunning || canStart);
    if (isRunning) {
        ui->TaskControlButton->setProperty("controlState", "pause");
        ui->TaskControlButton->setIcon(QIcon(QStringLiteral(":/pause.svg")));
        ui->TaskControlButton->setToolTip(QStringLiteral("暂停任务"));
    } else if (canStart) {
        ui->TaskControlButton->setProperty("controlState", "play");
        ui->TaskControlButton->setIcon(QIcon(QStringLiteral(":/play.svg")));
        ui->TaskControlButton->setToolTip(status == pausedText() ? QStringLiteral("继续任务")
                                                                 : QStringLiteral("开始任务"));
    }
    ui->StatusLabel->style()->unpolish(ui->StatusLabel);
    ui->StatusLabel->style()->polish(ui->StatusLabel);
    ui->progressBar->style()->unpolish(ui->progressBar);
    ui->progressBar->style()->polish(ui->progressBar);
    ui->TaskControlButton->style()->unpolish(ui->TaskControlButton);
    ui->TaskControlButton->style()->polish(ui->TaskControlButton);
}

void TaskItem::setSpeedInfo(QString info)
{
    ui->SpeedInfo->setText(info);
}

QString TaskItem::getFileName()
{
    return ui->FileName->text();
}

QString TaskItem::getSpeedInfo()
{
    return ui->SpeedInfo->text();
}

int TaskItem::getTaskValue()
{
    return ui->progressBar->value();
}

QString TaskItem::getTaskStatus()
{
    return ui->StatusLabel->text();
}

bool TaskItem::isChecked()
{
    return ui->Check->isChecked();
}

void TaskItem::setChecked(bool checked)
{
    ui->Check->setChecked(checked);
}
