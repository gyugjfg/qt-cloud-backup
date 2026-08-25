#ifndef DOWNLOADMODULE_H
#define DOWNLOADMODULE_H

#include <QObject>

class NodeGateway;
class TaskCreationGateway;
class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;
class QWidget;

/**
 * @brief 下载前置业务模块。
 *
 * 模块在 GUI 线程读取下载节点选择器和文件树，完成节点、勾选项、保存
 * 目录及重名确认后，通过 TaskCreationGateway 登记任务。所有注入对象均
 * 为非拥有指针；取消对话框、依赖不可用或没有可登记任务时直接返回，
 * 由已有消息提示维持用户可见语义。
 */
class DownloadModule : public QObject
{
    Q_OBJECT

public:
    /** 注入下载页控件和两个窄 Gateway；所有参数均由组合根拥有。 */
    explicit DownloadModule(QComboBox *downloadNodeCombo,
                            QWidget *messageParent,
                            NodeGateway *nodeGateway,
                            TaskCreationGateway *taskCreationGateway,
                            QString *lastSavePath,
                            QObject *parent = nullptr);

    /**
     * @brief 收口下载前置校验，并为当前勾选项创建下载任务。
     * @param treeWidget 当前下载页对应的文件树。
     */
    void createDownloadTasks(QTreeWidget *treeWidget);

signals:
    /** 每成功登记一个下载任务后回传 taskId 和展示文件名。 */
    void downloadTaskCreated(const QString &taskId, const QString &fileName);
    /** 当前批量任务已创建，页面可切换到任务页。 */
    void switchToTaskPageRequested();

private:
    /**
     * @brief 收集当前树控件中被勾选的下载项。
     * @param treeWidget 当前下载页文件树。
     * @return 当前树顶层中处于 Checked 状态的项；输入为空时返回空列表。
     */
    QList<QTreeWidgetItem*> checkedDownloadItems(QTreeWidget *treeWidget) const;
    /**
     * @brief 校验下载节点是否可用，并返回当前节点 ID。
     * @param nodeId 返回当前有效节点 ID。
     * @return 当前下载节点是否处于可下载状态；失败时 nodeId 可能为空。
     */
    bool ensureDownloadNodeReady(QString &nodeId) const;
    /**
     * @brief 让用户选择下载保存目录，并回写最近一次目录。
     * @return 最终选择的保存目录；取消或不可写时返回空串。
     */
    QString selectDownloadSaveDirectory();
    /**
     * @brief 处理本地同名文件的覆盖决策。
     * @param saveFilePath 本地目标文件路径。
     * @param decision 返回覆盖、跳过或取消决策。
     * @return 是否允许当前创建链继续执行。
     */
    bool confirmDownloadOverwrite(const QString &saveFilePath, QString &decision) const;
    /**
     * @brief 批量创建当前勾选文件对应的下载任务。
     * @param selectedItems 当前勾选文件项。
     * @param nodeId 当前下载节点 ID。
     * @param savePath 本地保存目录。
     * @param canceledByUser 返回用户是否主动取消。
     * @param skipCount 返回因重名被跳过的文件数。
     * @return 已向任务创建端口提交的任务数；用户取消会提前结束后续项。
     */
    int enqueueDownloadTasks(const QList<QTreeWidgetItem*> &selectedItems,
                             const QString &nodeId,
                             const QString &savePath,
                             bool &canceledByUser,
                             int &skipCount);

    QComboBox *m_downloadNodeCombo = nullptr;
    QWidget *m_messageParent = nullptr;
    NodeGateway *m_nodeGateway = nullptr;
    TaskCreationGateway *m_taskCreationGateway = nullptr;
    QString *m_lastSavePath = nullptr;
};

#endif // DOWNLOADMODULE_H
