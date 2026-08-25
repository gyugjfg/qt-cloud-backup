/**
 * @file DownloadModule.cpp
 * @brief 下载前置校验、保存目录选择和任务登记实现。
 *
 * 流程固定为“节点可用性 -> 勾选文件 -> 保存目录 -> 重名决策 ->
 * TaskCreationGateway 登记”。模块只在 GUI 线程访问控件，网络传输由
 * 任务层异步启动。
 */
#include "DownloadModule.h"

#include "NodeGateway.h"
#include "TaskCreationGateway.h"

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace {
QString sanitizedTransferFileName(const QString &fileName)
{
    QString text = fileName.trimmed();
    while (!text.isEmpty()) {
        const QChar firstChar = text.front();
        if (firstChar.isLetterOrNumber() || firstChar == '.' || firstChar == '_' || firstChar == '-') {
            break;
        }
        text.remove(0, 1);
        text = text.trimmed();
    }
    return text;
}

bool isDirectoryDownloadItem(const QTreeWidgetItem *item)
{
    if (!item) {
        return false;
    }

    const bool roleSaysDirectory = item->data(1, Qt::UserRole).toBool();
    const QString typeText = item->text(3).trimmed();
    const QString nameText = item->text(0).trimmed();

    if (typeText == QStringLiteral("目录")) {
        return true;
    }

    if (nameText.startsWith(QStringLiteral("📁 "))) {
        return true;
    }

    return roleSaysDirectory && (typeText.isEmpty() || typeText == QStringLiteral("文件夹"));
}
}

DownloadModule::DownloadModule(QComboBox *downloadNodeCombo,
                               QWidget *messageParent,
                               NodeGateway *nodeGateway,
                               TaskCreationGateway *taskCreationGateway,
                               QString *lastSavePath,
                               QObject *parent)
    : QObject(parent)
    , m_downloadNodeCombo(downloadNodeCombo)
    , m_messageParent(messageParent)
    , m_nodeGateway(nodeGateway)
    , m_taskCreationGateway(taskCreationGateway)
    , m_lastSavePath(lastSavePath)
{
    // 下载模块统一收口节点检查、保存路径和覆盖确认，不把这些分支散回主页。
}

/**
 * @brief 收口下载前置校验，并为当前勾选文件创建下载任务。
 * @param treeWidget 当前下载页文件树。
 */
void DownloadModule::createDownloadTasks(QTreeWidget *treeWidget)
{
    QString nodeId;
    if (!ensureDownloadNodeReady(nodeId)) {
        return;
    }

    if (!treeWidget) {
        QMessageBox::warning(m_messageParent,
                             QStringLiteral("警告"),
                             QStringLiteral("当前节点文件列表不可用"));
        return;
    }

    const QList<QTreeWidgetItem*> selectedItems = checkedDownloadItems(treeWidget);
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(m_messageParent,
                             QStringLiteral("警告"),
                             QStringLiteral("请先选择要下载的文件"));
        return;
    }

    for (QTreeWidgetItem *item : selectedItems) {
        if (isDirectoryDownloadItem(item)) {
            QMessageBox::warning(m_messageParent,
                                 QStringLiteral("警告"),
                                 QStringLiteral("当前版本暂不支持下载文件夹，请只选择文件"));
            return;
        }
    }
    // 选择保存目录是同步模态步骤；取消或不可写时不进入任务登记阶段。
    const QString savePath = selectDownloadSaveDirectory();
    if (savePath.isEmpty()) {
        return;
    }

    bool canceledByUser = false;
    int skipCount = 0;
    const int addedTaskCount = enqueueDownloadTasks(selectedItems, nodeId, savePath, canceledByUser, skipCount);

    if (addedTaskCount > 0) {
        emit switchToTaskPageRequested();
        if (canceledByUser) {
            QMessageBox::information(m_messageParent, QStringLiteral("提示"), QStringLiteral("下载已取消"));
        } else if (skipCount > 0) {
            QMessageBox::information(m_messageParent, QStringLiteral("提示"), QStringLiteral("部分文件已存在，已跳过重复任务"));
        }
        return;
    }

    if (canceledByUser) {
        QMessageBox::information(m_messageParent, QStringLiteral("提示"), QStringLiteral("下载已取消"));
    } else if (skipCount > 0) {
        QMessageBox::information(m_messageParent, QStringLiteral("提示"), QStringLiteral("所选文件均已存在，未新增下载任务"));
    } else {
        QMessageBox::information(m_messageParent, QStringLiteral("提示"), QStringLiteral("未新增下载任务"));
    }
}

QList<QTreeWidgetItem*> DownloadModule::checkedDownloadItems(QTreeWidget *treeWidget) const
{
    QList<QTreeWidgetItem*> selectedItems;
    if (!treeWidget) {
        return selectedItems;
    }

    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);
        if (item && item->checkState(0) == Qt::Checked) {
            selectedItems.append(item);
        }
    }

    return selectedItems;
}

/**
 * @brief 校验当前下载节点是否可用。
 * @param nodeId 返回当前有效节点 ID。
 * @return 下载节点是否已经选中且在线。
 */
bool DownloadModule::ensureDownloadNodeReady(QString &nodeId) const
{
    if (!m_downloadNodeCombo) {
        return false;
    }

    const int currentIndex = m_downloadNodeCombo->currentIndex();
    if (currentIndex <= 0) {
        QMessageBox::warning(m_messageParent, QStringLiteral("警告"), QStringLiteral("请先选择节点"));
        return false;
    }

    nodeId = m_downloadNodeCombo->itemData(currentIndex).toString();
    if (!m_nodeGateway) {
        return !nodeId.isEmpty();
    }

    const NodeGateway::NodeInfo nodeInfo = m_nodeGateway->nodeInfo(nodeId);
    if (nodeInfo.status != 1) {
        QMessageBox::warning(m_messageParent, QStringLiteral("警告"), QStringLiteral("所选节点当前离线"));
        return false;
    }

    return true;
}

/**
 * @brief 让用户选择下载保存目录，并回写最近一次目录。
 * @return 最终选择的保存目录；取消或不可写时返回空串。
 */
QString DownloadModule::selectDownloadSaveDirectory()
{
    QFileDialog dialog(m_messageParent, QStringLiteral("选择保存目录"));
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setDirectory(m_lastSavePath ? *m_lastSavePath : QString());

    if (dialog.exec() != QDialog::Accepted) {
        return QString();
    }

    const QString savePath = dialog.selectedFiles().value(0);
    if (savePath.isEmpty()) {
        return QString();
    }

    QFileInfo savePathInfo(savePath);
    if (!savePathInfo.isWritable()) {
        QMessageBox::warning(m_messageParent, QStringLiteral("警告"), QStringLiteral("所选保存目录不可写"));
        return QString();
    }

    if (m_lastSavePath) {
        *m_lastSavePath = savePath;
    }
    return savePath;
}

/**
 * @brief 处理本地重名文件的覆盖决策。
 * @param saveFilePath 本地目标文件路径。
 * @param decision 返回覆盖、跳过或取消决策。
 * @return 是否允许当前创建链继续执行。
 */
bool DownloadModule::confirmDownloadOverwrite(const QString &saveFilePath, QString &decision) const
{
    if (!QFile::exists(saveFilePath)) {
        decision = QStringLiteral("overwrite");
        return true;
    }

    QMessageBox msgBox(m_messageParent);
    msgBox.setWindowTitle(QStringLiteral("确认覆盖"));
    msgBox.setText(QStringLiteral("目标目录中已存在同名文件，是否覆盖？"));
    QPushButton *overwriteButton = msgBox.addButton(QStringLiteral("覆盖"), QMessageBox::AcceptRole);
    QPushButton *skipButton = msgBox.addButton(QStringLiteral("跳过"), QMessageBox::DestructiveRole);
    QPushButton *cancelButton = msgBox.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    msgBox.exec();

    if (msgBox.clickedButton() == overwriteButton) {
        decision = QStringLiteral("overwrite");
        return true;
    }
    if (msgBox.clickedButton() == skipButton) {
        decision = QStringLiteral("skip");
        return true;
    }

    Q_UNUSED(cancelButton);
    decision = QStringLiteral("cancel");
    return false;
}

/**
 * @brief 批量创建当前勾选文件对应的下载任务。
 * @param selectedItems 当前勾选文件项。
 * @param nodeId 当前下载节点 ID。
 * @param savePath 本地保存目录。
 * @param canceledByUser 返回用户是否主动取消。
 * @param skipCount 返回因重名被跳过的文件数。
 * @return 实际创建成功的任务数。
 */
int DownloadModule::enqueueDownloadTasks(const QList<QTreeWidgetItem*> &selectedItems,
                                         const QString &nodeId,
                                         const QString &savePath,
                                         bool &canceledByUser,
                                         int &skipCount)
{
    int addedTaskCount = 0;

    for (QTreeWidgetItem *item : selectedItems) {
        if (!item) {
            continue;
        }

        const QString remoteFilePath = item->data(0, Qt::UserRole).toString();
        const QString fileName = sanitizedTransferFileName(remoteFilePath.section("/", -1));
        const QString saveFilePath = savePath + "/" + fileName;

        QString decision;
        if (!confirmDownloadOverwrite(saveFilePath, decision)) {
            canceledByUser = true;
            break;
        }
        if (decision == QStringLiteral("skip")) {
            ++skipCount;
            continue;
        }

        if (!m_taskCreationGateway || !m_taskCreationGateway->isAvailable()) {
            continue;
        }

        const QString taskId = m_taskCreationGateway->createDownloadTask(remoteFilePath,
                                                                          saveFilePath,
                                                                          nodeId,
                                                                          0);
        emit downloadTaskCreated(taskId, fileName);
        ++addedTaskCount;
    }

    return addedTaskCount;
}
