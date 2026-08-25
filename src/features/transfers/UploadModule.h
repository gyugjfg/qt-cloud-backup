#ifndef UPLOADMODULE_H
#define UPLOADMODULE_H

#include <QObject>

class QComboBox;
class QWidget;
class NodeGateway;
class TaskCreationGateway;

/**
 * @brief 上传前置业务模块。
 *
 * 模块读取上传节点选择器，校验节点存在且在线，再把文件路径逐项交给
 * TaskCreationGateway。文件列表、勾选状态和控件切换由 UploadController
 * 负责；注入的控件、Gateway 和消息父窗口均为借用指针，调用在 GUI 线程。
 */
class UploadModule : public QObject
{
    Q_OBJECT

public:
    /** 注入上传节点选择器、节点能力和任务创建能力；不拥有这些对象。 */
    explicit UploadModule(QComboBox *uploadNodeCombo,
                          NodeGateway *nodeGateway,
                          TaskCreationGateway *taskCreationGateway,
                          QWidget *messageParent,
                          QObject *parent = nullptr);

    /** 校验上传节点选择器是否存在可选项；失败时显示提示并返回 false。 */
    bool ensureUploadNodeSelected() const;
    /** 返回当前选择器对应的节点 ID；未选中或控件缺失时返回空字符串。 */
    QString selectedUploadNodeId() const;
    /**
     * @brief 对当前文件集合做节点校验并创建上传任务。
     * @param filePaths 待上传的本地文件路径集合；空路径会被跳过。
     */
    void createUploadTasks(const QStringList &filePaths);

signals:
    /** 每成功登记一个上传任务后回传 taskId 和本地路径。 */
    void uploadTaskCreated(const QString &taskId, const QString &filePath);
    /** 当前批量任务已创建，页面可切换到任务页。 */
    void switchToTaskPageRequested();

private:
    QComboBox *m_uploadNodeCombo = nullptr; ///< 借用的节点选择器。
    NodeGateway *m_nodeGateway = nullptr; ///< 借用的节点查询端口。
    TaskCreationGateway *m_taskCreationGateway = nullptr; ///< 借用的任务创建端口。
    QWidget *m_messageParent = nullptr; ///< 借用的消息框父窗口。
};

#endif // UPLOADMODULE_H
