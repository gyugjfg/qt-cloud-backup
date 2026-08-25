#ifndef TASKCREATIONGATEWAY_H
#define TASKCREATIONGATEWAY_H

#include <QObject>
#include <QString>

class TaskManager;

/**
 * @brief 上传/下载输入模块使用的任务创建窄端口。
 *
 * Gateway 只借用 TaskManager，不拥有任务仓库，也不暴露状态机和快照。
 * 调用是同步的：成功返回 TaskManager 生成的 taskId，依赖未注入时返回
 * 空字符串；任务真正启动由 TaskModule/TaskManager 后续完成。
 */
class TaskCreationGateway final : public QObject
{
    Q_OBJECT

public:
    /** 注入任务仓库；指针由组合根保证生命周期，Gateway 不负责释放。 */
    explicit TaskCreationGateway(TaskManager *taskManager, QObject *parent = nullptr);

    /** 返回任务创建依赖是否已注入。 */
    bool isAvailable() const;
    /** 创建上传任务并返回唯一 taskId；依赖不可用时返回空字符串。 */
    QString createUploadTask(const QString &localPath, const QString &nodeId) const;
    /** 创建下载任务并返回唯一 taskId；fileSize 作为初始总大小传入。 */
    QString createDownloadTask(const QString &remoteFilePath,
                               const QString &savePath,
                               const QString &nodeId,
                               qint64 fileSize = 0) const;

private:
    TaskManager *m_taskManager = nullptr; ///< 借用的任务仓库，不由 Gateway 释放。
};

#endif // TASKCREATIONGATEWAY_H
