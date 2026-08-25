#ifndef TRANSFERSERVICE_H
#define TRANSFERSERVICE_H

#include "TransferTypes.h"
#include "TransferControlState.h"

#include <QObject>
#include <QMutex>
#include <QQueue>

#include <atomic>

// 传输执行层：负责上传下载落地、队列调度和暂停/取消控制。
// 它只依赖共享传输契约，不依赖 features/transfers 下的任务业务对象。
class TransferService : public QObject
{
    Q_OBJECT

public:
    enum class TransferKind {
        Upload,
        Download
    };

    enum class TransferControlAction {
        Pause,
        Cancel
    };

    struct TransferRequest {
        TransferKind type = TransferKind::Download;
        QString filePath;
        QString fileName;
        QString savePath;
        QString nodeId;
        QString taskId;
        QString nodeIp;
        int nodePort = 0;
        int threadCount = 1;
        qint64 startOffset = 0;
    };

    explicit TransferService(QObject *parent = nullptr);
    ~TransferService();

    /**
     * @brief 同步执行上传任务。
     * @param request 已补齐节点和路径信息的上传请求。
     * @return 上传是否成功完成。
     */
    bool fileUpload(const TransferRequest &request);
    /**
     * @brief 同步执行下载任务。
     * @param request 已补齐节点和路径信息的下载请求。
     * @return 下载是否成功完成。
     */
    bool fileDownload(const TransferRequest &request);
    /**
     * @brief 将传输请求放入异步队列并触发调度。
     * @param request 待执行的传输请求。
     */
    void startTransferAsync(const TransferRequest &request);
    // 统一传输控制入口，暂停/取消的旧接口仅作为兼容包装保留。
    void controlTransfer(const QString &taskId,
                         TransferKind kind,
                         TransferControlAction action);

    /** 兼容包装：请求暂停下载。 */
    void pauseDownload(const QString &taskId);
    /** 兼容包装：请求取消下载。 */
    void cancelDownload(const QString &taskId);
    /** 兼容包装：请求暂停上传。 */
    void pauseUpload(const QString &taskId);
    /** 兼容包装：请求取消上传。 */
    void cancelUpload(const QString &taskId);

signals:
    /** 按 taskId 回传归一化后的传输进度。 */
    void taskProgressChanged(const QString &taskId, int progress,
                             qint64 transferredSize, qint64 totalSize, double speed);
    /** 回传与 TaskManager 状态码兼容的传输状态。 */
    void taskStatusChanged(const QString &taskId, int status);
    /** 回传可直接展示或记录的传输错误。 */
    void taskError(const QString &taskId, const QString &errorMessage);

private:
    enum class TaskControlResult {
        Continue,
        Paused,
        Canceled
    };

    struct TransferOutcome {
        bool success = false;
        QString taskId;
        qint64 committedBytes = 0;
        qint64 totalBytes = 0;
    };

    bool fileUploadResumable(const TransferRequest &request);
    bool fileDownloadResumable(const TransferRequest &request);
    QString resolvedTaskId(const TransferRequest &request, TransferKind kind) const;
    bool validateTransferRequest(const TransferRequest &request, const QString &taskId);
    void emitTransferFailure(const QString &taskId, const QString &errorMessage);
    bool finishCanceledTransfer(const QString &taskId, bool isUpload);
    bool finishPausedTransfer(const QString &taskId, bool isUpload);
    bool handleSocketSendFailure(const QString &taskId, bool isUpload, const QString &errorMessage);
    void enqueueTransferTask(const TransferRequest &request);
    void tryScheduleTaskProcessingLocked();
    void executeTransferTask(const TransferRequest &request);
    void processTaskQueue();

    TaskControlResult checkTaskControlState(const QString &taskId, bool isUpload) const;
    void cleanupTaskControlState(const QString &taskId, bool isUpload);
    void requestTaskControl(const QString &taskId, bool isUpload, bool pauseRequest);

    void emitTaskProgressSignal(const QString &taskId, int progress,
                                qint64 transferred, qint64 total, double speed);
    void emitTaskStatusSignal(const QString &taskId, int status);
    void emitTaskErrorSignal(const QString &taskId, const QString &errorMessage);

    QQueue<TransferRequest> m_taskQueue;
    mutable QMutex m_taskQueueMutex;
    bool m_taskProcessing;

    // 生命周期边界：析构开始后拒绝新任务，并让已投递 worker 尽快收敛。
    std::atomic_bool m_shutdownRequested;

    mutable QMutex m_taskControlMutex;
    TransferControlState m_transferControlState;
};

#endif // TRANSFERSERVICE_H
