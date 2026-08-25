#ifndef TRANSFERCONTROLSTATE_H
#define TRANSFERCONTROLSTATE_H

#include <QList>
#include <QMap>
#include <QString>

/**
 * @brief 维护单个 TransferService 的传输控制状态。
 *
 * 该对象只保存任务方向、活动 socket 和暂停/取消请求，不负责加锁、关闭
 * socket 或发送 Qt 信号。调用方在持有外层控制锁时使用它，从而把状态规则
 * 与 TransferService 的文件 I/O、线程池和协议代码分开。
 */
class TransferControlState final
{
public:
    enum class Direction {
        Upload,
        Download
    };

    enum class Action {
        Pause,
        Cancel
    };

    enum class Result {
        Continue,
        Paused,
        Canceled
    };

    /** 注册任务当前活动 socket；重复注册覆盖旧句柄，保持原有 map 语义。 */
    void registerSocket(Direction direction, const QString &taskId, int socket);

    /**
     * @brief 读取控制结果。
     * @param shutdownRequested 服务是否已进入析构关闭阶段。
     * @return 关闭优先于取消，取消优先于暂停；无活动 socket 视为暂停。
     */
    Result check(Direction direction, const QString &taskId, bool shutdownRequested) const;

    /**
     * @brief 写入一次暂停/取消请求并摘除活动 socket。
     * @return 被摘除的 socket，若当前没有活动 socket 则返回 -1。
     */
    int request(Direction direction, const QString &taskId, Action action);

    /** 清理任务的 socket 和控制请求，保持任务终态后的原有收敛语义。 */
    void cleanup(Direction direction, const QString &taskId);

    /** 返回方向下所有仍登记的 socket，供服务析构时统一关闭。 */
    QList<int> activeSockets(Direction direction) const;

    /** 清空所有方向的运行时状态。 */
    void clear();

private:
    struct DirectionState {
        QMap<QString, int> sockets;
        QMap<QString, int> pauseRequests;
        QMap<QString, int> cancelRequests;
    };

    DirectionState &state(Direction direction);
    const DirectionState &state(Direction direction) const;

    DirectionState m_upload;
    DirectionState m_download;
};

#endif // TRANSFERCONTROLSTATE_H
