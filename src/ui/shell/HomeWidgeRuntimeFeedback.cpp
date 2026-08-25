/*
 * 主窗口组合根实现：装配模块、路由页面和汇总全局 UI 反馈。
 * 跨模块连接属于组合职责；页面校验、统计和弹窗仍有存量逻辑，不能视为纯装配器。
 */
#include "FileItem.h"
#include "HomeDownloadSelectionPolicy.h"
#include "HomeFileRowPresentation.h"
#include "HomeTaskErrorPolicy.h"
#include "HomeTaskStatusPolicy.h"
#include "HomeWidge.h"
#include "Database.h"
#include "NetWork.h"
#include "DirectoryGateway.h"
#include "NodeGateway.h"
#include "TaskTransferGateway.h"
#include "TaskNodeNameGateway.h"
#include "TaskCreationGateway.h"
#include "TaskManager.h"
#include "FileBrowser.h"
#include "TaskItem.h"
#include "NodeDialog.h"
#include "DirectoryNavigator.h"
#include "DirectoryModule.h"
#include "DirectoryPageController.h"
#include "NodeModule.h"
#include "NodePageController.h"
#include "UploadModule.h"
#include "UploadController.h"
#include "DownloadModule.h"
#include "DownloadController.h"
#include "TaskListController.h"
#include "TaskModule.h"
#include "ui_HomeWidge.h"
#include <QSize>
#include <QColor>
#include <QPalette>

#include <QFileDialog>
#include <QLineEdit>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <QTabBar>
#include <algorithm>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QAbstractItemModel>

void HomeWidge::handleFileListUpdated(const QString &nodeId, const QList<NetworkFileInfo> &fileList)
{
    // 自动同步仍按页签维度生效，只回刷开启同步且命中的页签。
    for (int i = 0; i < ui->NodeTabWidget->count(); i++) {
        QWidget *tabWidget = ui->NodeTabWidget->widget(i);
        QTreeWidget *treeWidget = tabWidget->findChild<QTreeWidget*>();
        QCheckBox *autoSyncCheckBox = tabWidget->findChild<QCheckBox*>("autoSyncCheckBox");

        bindDownloadTreeSelectionSync(treeWidget);

        if (treeWidget && autoSyncCheckBox) {
            if (autoSyncCheckBox->isChecked()) {
                QString tabText = ui->NodeTabWidget->tabText(i);
                if (tabText.contains(nodeId)) {
                    treeWidget->clear();
                    for (const NetWork::FileInfo &fileInfo : fileList) {
                        const HomeFileRowPresentation::Row row =
                            HomeFileRowPresentation::fromFileInfo(fileInfo);
                        QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
                        item->setCheckState(0, Qt::Unchecked);

                        item->setText(0, row.displayName);
                        item->setText(1, row.sizeText);
                        item->setText(2, row.modifiedText);
                        item->setText(3, row.typeText);
                        item->setData(0, Qt::UserRole, row.filePath);
                        item->setData(1, Qt::UserRole, row.isDirectory);
                        treeWidget->addTopLevelItem(item);
                    }

                    if (fileList.isEmpty()) {
                        QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
                        item->setText(0, "暂无文件");
                        item->setText(1, "");
                        item->setText(2, "");
                        item->setText(3, "");
                        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
                        item->setForeground(0, QColor(150, 150, 150));
                    }

                    if (tabWidget == ui->NodeTabWidget->currentWidget()) {
                        syncDownloadSelectAllState();
                    }
                }
            }
        }
    }
}

void HomeWidge::handleTaskProgressChanged(const QString &taskId, int progress, qint64 transferredSize, qint64 totalSize, double speed)
{
    TaskManager::TaskSnapshot taskSnapshot;
    if (!m_taskManager->getTaskSnapshot(taskId, taskSnapshot)) {
        return;
    }

    Q_UNUSED(progress);

    if (!m_taskModule) {
        return;
    }

    m_taskModule->updateTaskProgressDisplay(taskId, transferredSize, totalSize, speed);
}

/**
 * @brief 处理任务主线回流的状态变化，并补主页级统计与总结反馈。
 * @param taskId 发生变化的任务 ID。
 * @param status 任务当前状态值。
 */
void HomeWidge::handleTaskStatusChanged(const QString &taskId, int status)
{
    TaskManager::TaskSnapshot taskSnapshot;
    const bool taskFound = m_taskManager->getTaskSnapshot(taskId, taskSnapshot);

    // 主页这里只补全局汇总和兜底展示，任务项迁移逻辑继续留在 TaskModule。
    if (taskFound) {
        if (m_taskModule) {
            m_taskModule->refreshTaskDisplay(taskId);
        }
        if (HomeTaskStatusPolicy::isTerminal(status)) {
            // 任务列表迁移已经下沉到 TaskModule，主页这里只补汇总统计和全局反馈。
            TaskModule::FinishedTaskInfo finishedTaskInfo;
            if (m_taskModule && m_taskModule->handleTaskCompletion(taskId, status, finishedTaskInfo) && finishedTaskInfo.moved) {
                recordTaskCompletion(finishedTaskInfo.fileName,
                                     finishedTaskInfo.nodeName,
                                     finishedTaskInfo.isDownload,
                                     finishedTaskInfo.status);
            }
        }
    }

    // 某些终态可能在运行中项移走后才回流到主页，这里继续做一次兜底迁移。
    if (!taskFound && HomeTaskStatusPolicy::isTerminal(status)) {
        if (!m_taskModule || !m_taskModule->taskExistsInFinishedList(taskId)) {
            TaskModule::FinishedTaskInfo finishedTaskInfo;
            if (m_taskModule && m_taskModule->handleTaskCompletion(taskId, status, finishedTaskInfo) && finishedTaskInfo.moved) {
                recordTaskCompletion(finishedTaskInfo.fileName,
                                     finishedTaskInfo.nodeName,
                                     finishedTaskInfo.isDownload,
                                     finishedTaskInfo.status);
            }
        }
    }

    if ((!m_taskModule || !m_taskModule->hasActiveTasks()) &&
        m_taskFeedbackSummary.hasRecords()) {
        showTaskSummary();
    }
}

/**
 * @brief 汇总当前批次上传/下载任务结果，并以主页级弹窗给出总结。
 */
void HomeWidge::showTaskSummary()
{
    // 汇总弹窗仍留在主页，文案拼装已经下沉到纯结果对象。
    if (!m_taskFeedbackSummary.hasRecords()) {
        return;
    }

    QMessageBox::information(this,
                             QString::fromUtf8(u8"\u4efb\u52a1\u5b8c\u6210"),
                             m_taskFeedbackSummary.message());
    m_taskFeedbackSummary.clear();
}

