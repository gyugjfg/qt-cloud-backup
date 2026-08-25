#ifndef TRANSFERTYPES_H
#define TRANSFERTYPES_H

#include <QString>
#include <QtGlobal>

// 传输方向是客户端任务、网络门面和底层执行器之间共享的值对象契约。
enum class NetworkTransferType {
    Upload,
    Download
};

// 控制意图只表达暂停或取消，不在网络层推导任务状态。
enum class NetworkTransferControlAction {
    Pause,
    Cancel
};

// 网络回流与任务状态机共享的状态码。
// 任务层通过类型别名使用它，避免 core 和 feature 各自维护一份数值枚举。
enum class NetworkTransferStatus {
    Waiting = 0,
    Running = 1,
    Paused = 2,
    Completed = 3,
    Failed = 4,
    Canceled = 5
};

// 任务层下发给网络层的最小传输请求，所有路径和恢复偏移均使用明确字段传递。
struct NetworkTransferRequest {
    using Type = NetworkTransferType;

    NetworkTransferType type = NetworkTransferType::Download; ///< 上传或下载方向。
    QString filePath;      ///< 上传时的本地源文件路径。
    QString fileName;      ///< 下载时的远程文件名。
    QString savePath;      ///< 下载时的本地目标文件路径。
    QString nodeId;        ///< 目标节点唯一标识。
    QString taskId;        ///< 任务唯一标识，用于进度、状态和控制回流。
    int threadCount = 1;   ///< 下载并发度；任务主线默认 1，旧兼容入口可传入其他值。
    qint64 startOffset = 0; ///< 断点续传起始偏移，单位为字节。
};

#endif // TRANSFERTYPES_H
