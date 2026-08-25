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

void HomeWidge::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        isDrag = true;
        dVal = ev->globalPosition() - pos();
    }
}

void HomeWidge::mouseMoveEvent(QMouseEvent *ev)
{
    if(isDrag)
        move((ev->globalPosition()-dVal).toPoint());
}

void HomeWidge::mouseReleaseEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
        isDrag = false;
}

void HomeWidge::on_Closebutton_clicked()
{
    close();
}

void HomeWidge::on_Minibutton_clicked()
{
    showMinimized();
}

void HomeWidge::on_change_stackedWidget(int index)
{
    ui->FunctionList->setCurrentIndex(index);

    const QList<QPushButton*> navigationButtons = {
        ui->Upload, ui->Download, ui->Tasklist, ui->Nodelist
    };
    if (index >= 0 && index < navigationButtons.size()) {
        navigationButtons[index]->setChecked(true);
    }
}

void HomeWidge::startDownload()
{
    if (!m_taskModule) {
        return;
    }

    const TaskModule::ActionResult result = m_taskModule->startSelectedTasks();
    if (!result.message.isEmpty()) {
        showTaskControlMessage(QStringLiteral("提示"), result.message, result.warning);
    }
}


void HomeWidge::pauseDownload()
{
    if (!m_taskModule) {
        return;
    }

    const TaskModule::ActionResult result = m_taskModule->pauseSelectedTasks();
    if (!result.message.isEmpty()) {
        showTaskControlMessage(QStringLiteral("提示"), result.message, result.warning);
    }
}


void HomeWidge::cancelDownload()
{
    if (!m_taskModule) {
        return;
    }

    const TaskModule::ActionResult result = m_taskModule->cancelSelectedActiveTasks();
    if (!result.message.isEmpty()) {
        showTaskControlMessage(QStringLiteral("提示"), result.message, result.warning);
    }
}


void HomeWidge::onSelectAllCheckBoxChanged(int state)
{
    if (m_taskModule) {
        m_taskModule->setActiveTasksChecked(state == Qt::Checked);
    }
}

void HomeWidge::onSelectAllFinishedCheckBoxChanged(int state)
{
    if (m_taskModule) {
        m_taskModule->setFinishedTasksChecked(state == Qt::Checked);
    }
}

void HomeWidge::deleteSelectedFinishedTasks()
{
    if (m_taskModule) {
        m_taskModule->deleteSelectedFinishedTasks();
    }
}
//点击上传图标，选择文件上传（槽函数）
void HomeWidge::on_CheckFiles_clicked()
{
    if (!m_uploadController || !m_uploadController->ensureUploadNodeSelected()) {
        return;
    }

    const QStringList fileList = QFileDialog::getOpenFileNames(this);
    m_uploadController->prepareUploadFiles(fileList, true);
}


void HomeWidge::on_CancelUpload_clicked()
{
    if (m_uploadController) {
        m_uploadController->clearUploadFileList();
    }
}


QWidget *HomeWidge::getDefaultNodePage() const
{
    return ui->NodeTabWidget->widget(0);
}

void HomeWidge::resetDefaultNodePage()
{
    if (m_nodePageController) {
        m_nodePageController->resetDefaultNodePage();
    }
}

// 下载节点选择框改变时，刷新默认节点页面
void HomeWidge::on_DownloadNodeComboBox_currentIndexChanged(int index)
{
    if (index <= 0) {
        resetDefaultNodePage();
        return;
    }

    QString nodeId = ui->DownloadNodeComboBox->itemData(index).toString();
    QString nodeName = ui->DownloadNodeComboBox->currentText();

    if (nodeId.isEmpty()) {
        return;
    }

    if (m_nodePageController) {
        m_nodePageController->refreshDefaultNodePageForSelection(nodeId, nodeName);
    }
}

void HomeWidge::on_EnterUpload_clicked()
{
    if (!m_uploadController) {
        return;
    }

    m_taskFeedbackSummary.resetUploadTotalsForNewBatch();

    m_uploadController->startSelectedUploads();
}

void HomeWidge::changeCheckedItem(bool status, QWidget *item)
{
    if (m_uploadController) {
        m_uploadController->handleFileItemCheckedChanged(status, item);
    }

    FileItem *fileItem = static_cast<FileItem*>(item);
    if (!fileItem) {
        return;
    }
    if (status) {
        checkFileItems[fileItem->getItemIndex()] = item;
    } else {
        checkFileItems.remove(fileItem->getItemIndex());
    }
}


void HomeWidge::on_CheckAll_checkStateChanged(const Qt::CheckState &arg1)
{
    const bool checked = (arg1 == Qt::Checked);
    if (m_uploadController) {
        m_uploadController->setUploadFileItemsChecked(checked);
    }
    ui->CheckAll->setText(checked
                              ? QStringLiteral("\u53d6\u6d88\u5168\u9009")
                              : QStringLiteral("\u9009\u62e9\u5168\u90e8"));
}


void HomeWidge::on_RemoveAll_clicked()
{
    if (m_uploadController) {
        m_uploadController->clearUploadFileList();
    }
}


void HomeWidge::on_RemoveFiles_clicked()
{
    if (m_uploadController) {
        m_uploadController->removeCheckedUploadFiles();
    }
}

// 追加文件
void HomeWidge::on_AppendFiles_clicked()
{
    if (!m_uploadController || !m_uploadController->ensureUploadNodeSelected()) {
        return;
    }

    const QStringList fileList = QFileDialog::getOpenFileNames(this);
    m_uploadController->prepareUploadFiles(fileList, false);
}


void HomeWidge::handleBreadcrumbNavigate(const QString &nodeId, const QString &path)
{
    if (m_directoryPageController) {
        m_directoryPageController->handleBreadcrumbNavigate(nodeId, path);
    }
}

void HomeWidge::showTaskErrorMessage(const QString &taskId, const QString &errorMessage)
{
    const QString title = HomeTaskErrorPolicy::titleForTask(taskId);
    const QString errorKey = HomeTaskErrorPolicy::deduplicationKey(taskId, errorMessage);
    // 同一批底层错误可能同时从任务层和网络层回流，这里先做一次主页级弹窗去重。
    if (m_activeTaskErrorKeys.contains(errorKey)) {
        return;
    }

    m_activeTaskErrorKeys.insert(errorKey);
    QMessageBox::warning(this, title, errorMessage);
    QTimer::singleShot(0, this, [this, errorKey]() {
        m_activeTaskErrorKeys.remove(errorKey);
    });
}

void HomeWidge::on_NewFolder_clicked()
{
    int currentIndex = ui->DownloadNodeComboBox->currentIndex();
    if (currentIndex <= 0) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"警告"),
                             QString::fromUtf8(u8"请选择一个节点"));
        return;
    }

    if (m_directoryPageController) {
        m_directoryPageController->openDirectoryPageDialog(currentIndex);
    }
}

void HomeWidge::on_NewNode_clicked()
{
    NetWork::NodeInfo node;
    if (m_nodeModule && m_nodeModule->createNode(this, node)) {
        checkNodeItems = m_nodeModule->checkedNodeItems();
    }
}


void HomeWidge::on_RemoveNode_clicked()
{
    if (checkNodeItems.isEmpty()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"警告"),
                             QString::fromUtf8(u8"请先选择要移除的节点"));
        return;
    }

    const int ret = QMessageBox::question(this,
                                          QString::fromUtf8(u8"确认"),
                                          QString::fromUtf8(u8"确认要移除选中的节点吗？"));
    if (ret != QMessageBox::Yes) {
        return;
    }

    const QString currentNodeId = ui->DownloadNodeComboBox->currentData().toString();
    QList<QString> removedNodeIds;
    const bool removedCurrentNode = m_nodeModule
        ? m_nodeModule->removeSelectedNodes(currentNodeId, removedNodeIds)
        : false;
    if (m_nodeModule) {
        checkNodeItems = m_nodeModule->checkedNodeItems();
    }
    // 当前激活节点被删掉时，默认节点页也要一起退回空态。
    if (removedCurrentNode) {
        ui->DownloadNodeComboBox->setCurrentIndex(0);
        resetDefaultNodePage();
    }
}

void HomeWidge::on_ChangeNode_clicked()
{
    if (checkNodeItems.isEmpty()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"警告"),
                             QString::fromUtf8(u8"请先选择要修改配置的节点"));
        return;
    }

    if (checkNodeItems.size() > 1) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"警告"),
                             QString::fromUtf8(u8"只能选择一个节点进行修改"));
        return;
    }

    if (!m_nodeModule || !m_nodeModule->selectedSingleNodeListItem()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"警告"),
                             QString::fromUtf8(u8"无法获取节点信息"));
        return;
    }

    NetWork::NodeInfo updatedNode;
    if (m_nodeModule->updateSelectedNode(this, updatedNode)) {
        if (ui->DownloadNodeComboBox->currentData().toString() == updatedNode.nodeId) {
            if (m_nodePageController) {
                m_nodePageController->refreshDefaultNodePageForSelection(updatedNode.nodeId, updatedNode.nodeName);
            }
        }
    }
}


void HomeWidge::on_CheckAllNode_clicked()
{
    if (m_nodeModule) {
        m_nodeModule->setAllNodesChecked(ui->CheckAllNode->isChecked());
        checkNodeItems = m_nodeModule->checkedNodeItems();
    }
}
//
void HomeWidge::on_CheckAllFolder_clicked()
{
    bool isChecked = ui->CheckAllFolder->isChecked();
    QWidget *currentTab = ui->NodeTabWidget->currentWidget();
    if (!currentTab) {
        return;
    }
    QTreeWidget *treeWidget = currentTab->findChild<QTreeWidget*>();
    if (!treeWidget) {
        return;
    }

    for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);
        item->setCheckState(0, isChecked ? Qt::Checked : Qt::Unchecked);
    }

    syncDownloadSelectAllState();
}
//下载按钮槽函数
void HomeWidge::on_DownloadFolder_clicked()
{
    QTreeWidget *treeWidget = currentDownloadTree();
    if (m_downloadController) {
        m_downloadController->prepareDownloads(treeWidget);
    }
}


/**
 * @brief 处理目录自动同步回流，并按当前页签状态回刷下载树。
 * @param nodeId 发生更新的节点 ID。
 * @param fileList 该节点当前目录下的文件列表。
 */
void HomeWidge::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void HomeWidge::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void HomeWidge::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        return;
    }

    QStringList fileList;
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            fileList.append(url.toLocalFile());
        }
    }

    if (m_uploadController) {
        m_uploadController->prepareUploadFiles(fileList, true);
    }
    event->acceptProposedAction();
}

void HomeWidge::recordTaskCompletion(const QString &fileName, const QString &nodeName, bool isDownload, int status)
{
    const TaskFeedbackSummary::Kind kind = isDownload
        ? TaskFeedbackSummary::Kind::Download
        : TaskFeedbackSummary::Kind::Upload;
    const TaskFeedbackSummary::Outcome outcome =
        status == static_cast<int>(TaskManager::TaskStatus::Completed)
            ? TaskFeedbackSummary::Outcome::Success
            : (status == static_cast<int>(TaskManager::TaskStatus::Canceled)
                   ? TaskFeedbackSummary::Outcome::Canceled
                   : TaskFeedbackSummary::Outcome::Failed);
    m_taskFeedbackSummary.record(fileName, nodeName, kind, outcome);
}

QTreeWidget *HomeWidge::currentDownloadTree() const
{
    QWidget *currentTab = ui->NodeTabWidget->currentWidget();
    if (!currentTab) {
        return nullptr;
    }

    return currentTab->findChild<QTreeWidget*>();
}

void HomeWidge::showTaskControlMessage(const QString &title, const QString &message, bool warning) const
{
    if (warning) {
        QMessageBox::warning(const_cast<HomeWidge*>(this), title, message);
    } else {
        QMessageBox::information(const_cast<HomeWidge*>(this), title, message);
    }
}
