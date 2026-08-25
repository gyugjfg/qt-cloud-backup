#ifndef NETWORK_H
#define NETWORK_H

#include "NetworkTypes.h"
#include "TransferTypes.h"
#include "TransferService.h"

#include <QObject>

class DirectoryService;
class NodeService;

// 统一接入协调层：外部继续面向 NetWork，内部按节点 / 目录 / 传输三层复用服务对象。
class NetWork : public QObject
{
    Q_OBJECT

public:
    using NodeInfo = NetworkNodeInfo;
    using FileInfo = NetworkFileInfo;
    using TransferRequest = NetworkTransferRequest;
    using TransferControlAction = NetworkTransferControlAction;

    enum CMD {
        CREATE,
        DELETE
    };

    /** 创建统一网络门面；内部服务对象由 NetWork 自己拥有。 */
    explicit NetWork(QObject *parent = nullptr);
    ~NetWork();

    // 节点能力统一从这里进，具体实现继续下沉到 NodeService。
    /** 节点新增、删除、更新和查询入口。 */
    bool AddNode(const NodeInfo &node);
    bool RemoveNode(const QString &nodeId);
    bool UpdateNode(const NodeInfo &node);
    QList<NodeInfo> GetNodeList();
    NodeInfo GetNodeInfo(const QString &nodeId) const;
    bool CheckNodeStatus(const QString &nodeId);
    void CheckNodeStatusAsync(const QString &nodeId);

    // 上传 / 下载成对接口先保留兼容调用方，任务主线优先走统一请求入口。
    /** 兼容的同步上传入口；新任务主线使用 StartTransferAsync。 */
    bool FileUpload(QString filePath, const QString &nodeId, const QString &taskId = "", qint64 startOffset = 0);
    /** 兼容的异步上传入口。 */
    void FileUploadAsync(QString filePath, const QString &nodeId, const QString &taskId, qint64 startOffset = 0);

    /** 兼容的同步下载入口。 */
    bool FileDownload(QString fileName, QString filePath, const QString &nodeId,
                      const QString &taskId, int threadCount = 1, qint64 startOffset = 0);
    /** 兼容的异步下载入口。 */
    void FileDownloadAsync(QString fileName, QString filePath, const QString &nodeId,
                           const QString &taskId, int threadCount = 3, qint64 startOffset = 0);
    // 当前主推统一传输启动入口。
    /** 将统一传输请求转交 TransferService 的异步队列。 */
    void StartTransferAsync(const TransferRequest &request);
    // 当前主推统一传输控制入口。
    /** 将暂停/取消意图转交 TransferService。 */
    void ControlTransfer(const QString &taskId,
                         TransferRequest::Type type,
                         TransferControlAction action);

    // 目录浏览入口继续挂在 NetWork 上，便于页面层维持单一依赖面。
    QList<FileInfo> GetFileInfoList(const QString &nodeId, const QString &path = "./");
    FileInfo GetFileInfo(const QString &nodeId, const QString &filePath);
    void StartFileListSync(const QString &nodeId, int interval = 5000);
    void StopFileListSync(const QString &nodeId);

    bool Command(CMD cmd, QString name, const QString &nodeId);
    bool ChangeDirectory(const QString &nodeId, const QString &path);

    // 旧的暂停 / 取消接口仅保留兼容包装，内部统一落到 ControlTransfer。
    void pauseDownload(const QString &taskId);
    void cancelDownload(const QString &taskId);
    void pauseUpload(const QString &taskId);
    void cancelUpload(const QString &taskId);

signals:
    /** 统一转发传输进度，taskId 是唯一关联键。 */
    void taskProgressChanged(const QString &taskId, int progress,
                             qint64 transferredSize, qint64 totalSize, double speed);
    /** 统一转发传输状态码。 */
    void taskStatusChanged(const QString &taskId, int status);
    /** 统一转发传输失败文本。 */
    void taskError(const QString &taskId, const QString &errorMessage);
    /** 目录服务产生新快照时转发。 */
    void fileListUpdated(const QString &nodeId, const QList<FileInfo> &fileList);
    /** 异步节点在线探测结果。 */
    void nodeStatusChecked(const QString &nodeId, bool online);

private:
    TransferService::TransferRequest toServiceRequest(const TransferRequest &request) const;

    // 构造函数一次性创建并交给 QObject 父对象托管；门面方法不把它们当作可选依赖。
    TransferService *m_transferService;
    NodeService *m_nodeService;
    DirectoryService *m_directoryService;

    void sendTaskProgressSignal(const QString &taskId, int progress,
                                qint64 transferred, qint64 total, double speed);
    void sendTaskStatusSignal(const QString &taskId, int status);
    void sendTaskErrorSignal(const QString &taskId, const QString &errorMessage);
};

#endif // NETWORK_H
