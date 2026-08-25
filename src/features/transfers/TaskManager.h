#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "TaskTransferGateway.h"
#include "TransferTypes.h"

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QString>
#include <QtGlobal>

/**
 * @class TaskManager
 * @brief 任务主线协调对象，负责管理上传/下载任务生命周期与统一动作入口
 * 
 * 它属于任务管理主线的一部分，但不承担任务页模块本体语义。
 * 当前主要负责：
 * - 任务仓库维护
 * - 状态流转与统一任务视图
 * - 启动/暂停/取消等统一动作入口
 * - 通过 TaskTransferGateway 协调底层传输启动
 *
 * TaskManager 借用 TaskTransferGateway，不拥有 Gateway 或其 NetWork。
 * 任务表由 m_taskMutex 保护；公开方法可被传输回调线程调用，但页面
 * 信号仍按 Qt 对象连接规则投递给接收者。该类不直接执行 SQLite 持久化。
 */
class TaskManager : public QObject
{
    Q_OBJECT

public:
    /** 区分任务快照对应的上传或下载业务。 */
    enum class TaskKind {
        Download,
        Upload
    };

    /**
     * @brief 任务状态类型别名。
     * 状态值在网络回流和任务状态机之间共享，唯一契约定义位于 TransferTypes.h。
     */
    using TaskStatus = NetworkTransferStatus;

    /**
     * @brief 构造函数
     * @param transferGateway 任务专用传输端口，用于执行实际的下载/上传操作
     * @param parent 父对象
     */
    explicit TaskManager(TaskTransferGateway *transferGateway, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~TaskManager();

    /**
     * @struct DownloadTask
     * @brief 下载任务的进程内可变记录。
     *
     * 该结构只由 TaskManager 在持有 m_taskMutex 时读写，调用方通过
     * getDownloadTask() 获得副本，不应保存内部引用。
     */
    struct DownloadTask {
        QString taskId;        ///< 任务唯一标识符。
        QString fileName;      ///< 页面展示文件名。
        QString savePath;      ///< 本地保存路径。
        QString nodeId;        ///< 目标节点唯一标识。
        qint64 fileSize;       ///< 文件总大小，单位为字节；未知时为 0。
        int progress;          ///< 归一化展示进度，范围 0-100。
        int status;             ///< TaskStatus 的整数值。
        double speed;           ///< 最近一次速度，单位 KB/s。
        qint64 transferredBytes; ///< 已传输字节数，用于断点续传。
    };

    /**
     * @struct UploadTask
     * @brief 上传任务的进程内可变记录。
     *
     * 本地文件大小在创建时读取，恢复上传时会再次按文件信息校正；调用方
     * 应通过 getUploadTask() 获取副本，而不是依赖内部存储地址。
     */
    struct UploadTask {
        QString taskId;        ///< 任务唯一标识符。
        QString fileName;      ///< 页面展示文件名。
        QString localPath;     ///< 本地源文件路径。
        QString nodeId;        ///< 目标节点唯一标识。
        qint64 fileSize;       ///< 文件总大小，单位为字节；文件不存在时可能为 0。
        int progress;          ///< 归一化展示进度，范围 0-100。
        int status;             ///< TaskStatus 的整数值。
        double speed;           ///< 最近一次速度，单位 KB/s。
        qint64 transferredBytes; ///< 已传输字节数，用于断点续传。
    };

    /**
     * @struct TaskSnapshot
     * @brief 面向页面和跨模块查询的统一任务只读快照。
     * primaryPath 对下载表示保存路径，对上传表示本地源路径。
     */
    struct TaskSnapshot {
        QString taskId;             ///< 任务唯一标识。
        QString fileName;           ///< 页面展示名称。
        QString nodeId;             ///< 目标节点唯一标识。
        QString primaryPath;        ///< 当前任务的主路径，语义由 kind 决定。
        qint64 fileSize = 0;        ///< 文件总大小，单位为字节。
        qint64 transferredBytes = 0; ///< 已提交的字节数。
        int progress = 0;           ///< 归一化进度，范围 0-100。
        int status = 0;              ///< TaskStatus 的整数值。
        double speed = 0.0;          ///< 最近一次计算出的速度，单位 KB/s。
        TaskKind kind = TaskKind::Download; ///< 上传或下载类型。
    };

    /**
     * @brief 添加下载任务并以 Waiting 状态登记到内存任务表。
     * @param fileName 文件名
     * @param savePath 保存路径
     * @param nodeId 目标节点ID
     * @param fileSize 文件大小
     * @return 新生成的任务 ID；当前实现只要对象有效就返回非空 ID。
     */
    QString addDownloadTask(const QString &fileName, const QString &savePath,
                            const QString &nodeId, qint64 fileSize);
    
    /**
     * @brief 添加上传任务并以 Waiting 状态登记到内存任务表。
     * @param localPath 本地文件路径
     * @param nodeId 目标节点ID
     * @return 新生成的任务 ID；文件是否存在由后续传输阶段处理。
     */
    QString addUploadTask(const QString &localPath, const QString &nodeId);
    
    /**
     * @brief 删除任务并向底层发送取消意图（若 Gateway 可用）。
     * @param taskId 任务ID
     */
    void removeTask(const QString &taskId);
    
    /**
     * @brief 暂停任务并记录当前恢复偏移；不存在的任务静默返回。
     * @param taskId 任务ID
     */
    void pauseTask(const QString &taskId);
    
    /**
     * @brief 将任务置为 Canceled 并向底层发送取消意图；不存在的任务静默返回。
     * @param taskId 任务ID
     */
    void cancelTask(const QString &taskId);
    
    /**
     * @brief 启动 Waiting 或 Paused 任务；实际工作由 Gateway 异步下发。
     * @param taskId 任务ID
     */
    void resumeTask(const QString &taskId);
    /** 启动等待中或已暂停的任务；非法状态或任务不存在时返回 false。
     * Gateway 缺失时仍更新本地运行态，但不会产生网络请求。 */
    bool startTask(const QString &taskId);

    /**
     * @brief 获取下载任务副本。
     * @param taskId 任务ID
     * @param task 输出参数，成功时写入任务快照
     * @return 是否找到任务
     */
    bool getDownloadTask(const QString &taskId, DownloadTask &task) const;
    
    /**
     * @brief 获取上传任务副本。
     * @param taskId 任务ID
     * @param task 输出参数，成功时写入任务快照
     * @return 是否找到任务
     */
    bool getUploadTask(const QString &taskId, UploadTask &task) const;
    /** 获取统一任务快照；调用方不会拿到内部 QMap 的引用。 */
    bool getTaskSnapshot(const QString &taskId, TaskSnapshot &task) const;
    /** 判断任务是否为下载任务。 */
    bool isDownloadTask(const QString &taskId) const;
    /** 判断任务当前是否允许从等待/暂停态启动。 */
    bool canStartTask(const QString &taskId) const;
    /** 判断任务是否处于运行态。 */
    bool isTaskRunning(const QString &taskId) const;
    /** 判断任务是否已成功完成。 */
    bool isTaskCompleted(const QString &taskId) const;
    /** 获取页面使用的任务显示名称。 */
    QString taskDisplayName(const QString &taskId) const;
    /** 尝试启动任务并可选返回显示名称，供任务页批量动作使用。 */
    bool tryStartManagedTask(const QString &taskId, QString *displayName = nullptr);
    /** 尝试暂停任务，返回任务是否存在且已处理。 */
    bool tryPauseManagedTask(const QString &taskId);
    
    /**
     * @brief 获取所有任务ID
     * @return 任务ID列表
     */
    QList<QString> getAllTaskIds() const;

    /**
     * @brief 更新任务进度
     * @param taskId 任务ID
     * @param progress 进度（0-100）
     * @param transferredSize 已传输大小
     * @param totalSize 总大小
     * @param speed 传输速度（KB/s）
     */
    void updateTaskProgress(const QString &taskId, int progress,
                           qint64 transferredSize, qint64 totalSize, double speed);
    
    /**
     * @brief 接收网络层状态回流并更新内存快照。
     * @param taskId 任务 ID。
     * @param status NetworkTransferStatus 的整数值；此入口不执行合法转换校验，
     *               仅对暂停和终态同步恢复偏移。
     * 需要严格校验时使用 updateTaskStatusAtomically。
     */
    void updateTaskStatus(const QString &taskId, int status);
    /** 清零任务进度、速度和恢复偏移，不改变任务状态。 */
    void resetTaskProgress(const QString &taskId);
    /** 兼容的启动包装，统一转发到 startTask。 */
    bool tryStartTask(const QString &taskId);
    
    /**
     * @brief 判断任务状态机是否允许从 from 转换到 to。
     * @param from 原状态
     * @param to 目标状态
     * @return 是否为合法转换
     */
    static bool isValidTransition(TaskStatus from, TaskStatus to);
    
    /**
     * @brief 在任务锁内校验并原子化更新状态。
     * @param taskId 任务 ID。
     * @param newStatus 目标状态整数值。
     * @return 任务存在且状态转换合法时返回 true。
     * 该“原子”只覆盖进程内 QMap 与信号发送前的状态写入，不包含 SQLite 事务。
     */
    bool updateTaskStatusAtomically(const QString &taskId, int newStatus);

signals:
    /**
     * @brief 任务进度变化信号
     * @param taskId 任务ID
     * @param progress 进度（0-100）
     * @param transferredSize 已传输大小
     * @param totalSize 总大小
     * @param speed 传输速度（KB/s）
     */
    void taskProgressChanged(const QString &taskId, int progress,
                           qint64 transferredSize, qint64 totalSize, double speed);
    
    /**
     * @brief 任务状态变化信号
     * @param taskId 任务ID
     * @param status 状态码
     */
    void taskStatusChanged(const QString &taskId, int status);
    
    /**
     * @brief 任务错误信号
     * @param taskId 任务ID
     * @param error 错误信息
     */
    void taskError(const QString &taskId, const QString &error);

private:
    /**
     * @brief 在持锁状态下启动下载任务，并补齐恢复偏移。
     * @param task 待启动的下载任务对象。
     * @return 启动请求是否成功下发。
     */
    bool startDownloadTaskLocked(DownloadTask &task);
    /**
     * @brief 在持锁状态下启动上传任务，并补齐恢复偏移。
     * @param task 待启动的上传任务对象。
     * @return 启动请求是否成功下发。
     */
    bool startUploadTaskLocked(UploadTask &task);
    /** 根据下载快照构造不含页面对象的网络请求 DTO。 */
    TaskTransferGateway::TransferRequest buildDownloadTransferRequest(const DownloadTask &task, qint64 startOffset) const;
    /** 根据上传快照构造不含页面对象的网络请求 DTO。 */
    TaskTransferGateway::TransferRequest buildUploadTransferRequest(const UploadTask &task, qint64 startOffset) const;
    /** 以本地文件实际长度修正下载恢复偏移。调用时必须已持有任务锁。 */
    qint64 syncDownloadTransferredBytesLocked(DownloadTask &task);
    /** 以本地文件信息修正上传恢复偏移。调用时必须已持有任务锁。 */
    qint64 syncUploadTransferredBytesLocked(UploadTask &task);
    /** 根据已传输和总字节数计算 0-100 的展示进度。 */
    int progressFromBytes(qint64 transferredBytes, qint64 totalBytes) const;

    QMap<QString, DownloadTask> m_downloadTasks;  ///< 下载任务表，仅在锁内访问。
    QMap<QString, UploadTask> m_uploadTasks;      ///< 上传任务表，仅在锁内访问。
    mutable QMutex m_taskMutex;                   ///< 保护两张任务表的互斥锁。
    TaskTransferGateway *m_transferGateway;       ///< 借用的任务传输端口，不由本类释放。
    quint64 m_taskIdCounter;                      ///< 进程内任务 ID 计数器。
};

#endif // TASKMANAGER_H
