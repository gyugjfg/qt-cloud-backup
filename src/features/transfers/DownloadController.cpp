/**
 * @file DownloadController.cpp
 * @brief 下载页入口控制器实现。
 *
 * 控制器不创建任务，也不解释文件树数据；它只把页面事件转发给
 * DownloadModule，并转发模块发出的任务和页面切换信号。
 */
#include "DownloadController.h"

#include "DownloadModule.h"

#include <QTreeWidget>

DownloadController::DownloadController(QComboBox *downloadNodeCombo,
                                       QWidget *messageParent,
                                       DownloadModule *downloadModule,
                                       QObject *parent)
    : QObject(parent)
    , m_messageParent(messageParent)
    , m_downloadModule(downloadModule)
{
    Q_UNUSED(downloadNodeCombo);
    if (m_downloadModule) {
        connect(m_downloadModule, &DownloadModule::downloadTaskCreated,
                this, &DownloadController::downloadTaskCreated);
        connect(m_downloadModule, &DownloadModule::switchToTaskPageRequested,
                this, &DownloadController::switchToTaskPageRequested);
    }
}

void DownloadController::prepareDownloads(QTreeWidget *treeWidget)
{
    if (m_downloadModule) {
        m_downloadModule->createDownloadTasks(treeWidget);
    }
}
