/**
 * @file UploadModule.cpp
 * @brief 上传节点校验和任务登记实现。
 *
 * 模块不维护文件列表 UI，也不直接操作传输服务；校验通过后逐个调用
 * TaskCreationGateway，并把创建结果交给页面控制器继续编排。
 */
#include "UploadModule.h"

#include "NodeGateway.h"
#include "TaskCreationGateway.h"

#include <QComboBox>
#include <QMessageBox>

UploadModule::UploadModule(QComboBox *uploadNodeCombo,
                           NodeGateway *nodeGateway,
                           TaskCreationGateway *taskCreationGateway,
                           QWidget *messageParent,
                           QObject *parent)
    : QObject(parent)
    , m_uploadNodeCombo(uploadNodeCombo)
    , m_nodeGateway(nodeGateway)
    , m_taskCreationGateway(taskCreationGateway)
    , m_messageParent(messageParent)
{
}

bool UploadModule::ensureUploadNodeSelected() const
{
    if (m_uploadNodeCombo && m_uploadNodeCombo->currentIndex() > 0) {
        return true;
    }

    if (!m_uploadNodeCombo || m_uploadNodeCombo->count() <= 1) {
        QMessageBox::warning(m_messageParent,
                             QStringLiteral("警告"),
                             QStringLiteral("没有可用的备份节点，请先添加节点"));
        return false;
    }

    QMessageBox::warning(m_messageParent,
                         QStringLiteral("警告"),
                         QStringLiteral("请先在上传页面选择备份节点"));
    return false;
}

QString UploadModule::selectedUploadNodeId() const
{
    if (!m_uploadNodeCombo) {
        return QString();
    }

    const int currentIndex = m_uploadNodeCombo->currentIndex();
    if (currentIndex < 0) {
        return QString();
    }

    return m_uploadNodeCombo->itemData(currentIndex).toString();
}

/**
 * @brief 对当前文件集合做节点校验，并统一注册上传任务。
 * @param filePaths 待上传的本地文件路径集合。
 */
void UploadModule::createUploadTasks(const QStringList &filePaths)
{
    // 节点存在性和在线性统一在模块层兜底，页面层不重复做这一套判断。
    const QString nodeId = selectedUploadNodeId();
    if (nodeId.isEmpty()) {
        QMessageBox::warning(m_messageParent,
                             QStringLiteral("警告"),
                             QStringLiteral("请先选择备份节点"));
        return;
    }

    if (!m_nodeGateway) {
        return;
    }

    const NodeGateway::NodeInfo nodeInfo = m_nodeGateway->nodeInfo(nodeId);
    if (nodeInfo.nodeId.isEmpty()) {
        QMessageBox::warning(m_messageParent,
                             QStringLiteral("上传错误"),
                             QStringLiteral("所选备份节点不存在，无法上传"));
        return;
    }

    if (!m_nodeGateway->checkNodeStatus(nodeId)) {
        QMessageBox::warning(m_messageParent,
                             QStringLiteral("上传错误"),
                             QStringLiteral("所选备份节点当前离线，无法上传"));
        return;
    }

    for (const QString &filePath : filePaths) {
        if (filePath.isEmpty() || !m_taskCreationGateway || !m_taskCreationGateway->isAvailable()) {
            continue;
        }

        const QString taskId = m_taskCreationGateway->createUploadTask(filePath, nodeId);
        emit uploadTaskCreated(taskId, filePath);
    }

    if (!filePaths.isEmpty()) {
        emit switchToTaskPageRequested();
    }
}
