/*
 * 传输控制状态实现：只处理任务方向、socket 登记和暂停/取消请求优先级。
 * socket 的实际关闭仍由 TransferService 在控制锁范围内完成。
 */
#include "TransferControlState.h"

void TransferControlState::registerSocket(Direction direction,
                                          const QString &taskId,
                                          int socket)
{
    state(direction).sockets[taskId] = socket;
}

TransferControlState::Result TransferControlState::check(Direction direction,
                                                          const QString &taskId,
                                                          bool shutdownRequested) const
{
    if (shutdownRequested) {
        return Result::Canceled;
    }

    const DirectionState &current = state(direction);
    if (current.cancelRequests.value(taskId, 0) != 0) {
        return Result::Canceled;
    }
    if (current.pauseRequests.value(taskId, 0) != 0
        || !current.sockets.contains(taskId)) {
        return Result::Paused;
    }
    return Result::Continue;
}

int TransferControlState::request(Direction direction,
                                  const QString &taskId,
                                  Action action)
{
    DirectionState &current = state(direction);
    if (action == Action::Pause) {
        current.pauseRequests[taskId] = 1;
        current.cancelRequests.remove(taskId);
    } else {
        current.cancelRequests[taskId] = 1;
        current.pauseRequests.remove(taskId);
    }

    const auto socketIt = current.sockets.find(taskId);
    if (socketIt == current.sockets.end()) {
        return -1;
    }

    const int socket = socketIt.value();
    current.sockets.erase(socketIt);
    return socket;
}

void TransferControlState::cleanup(Direction direction, const QString &taskId)
{
    DirectionState &current = state(direction);
    current.sockets.remove(taskId);
    current.pauseRequests.remove(taskId);
    current.cancelRequests.remove(taskId);
}

QList<int> TransferControlState::activeSockets(Direction direction) const
{
    return state(direction).sockets.values();
}

void TransferControlState::clear()
{
    m_upload = {};
    m_download = {};
}

TransferControlState::DirectionState &TransferControlState::state(Direction direction)
{
    return direction == Direction::Upload ? m_upload : m_download;
}

const TransferControlState::DirectionState &TransferControlState::state(Direction direction) const
{
    return direction == Direction::Upload ? m_upload : m_download;
}
