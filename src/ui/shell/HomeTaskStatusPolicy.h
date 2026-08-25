#ifndef HOMETASKSTATUSPOLICY_H
#define HOMETASKSTATUSPOLICY_H

#include "TransferTypes.h"

/**
 * @brief 主页任务终态判定的纯规则边界。
 *
 * 该策略只解释任务状态码是否已经结束，不创建 QObject、窗口或异步回调。
 * 主页仍负责读取任务快照、迁移列表和展示反馈。
 */
class HomeTaskStatusPolicy
{
public:
    /**
     * @brief 判断状态是否属于完成、失败或取消终态。
     * @param status 传输状态的协议整数值。
     * @return 终态返回 true，等待、运行、暂停及未知值返回 false。
     */
    static bool isTerminal(int status)
    {
        return status == static_cast<int>(NetworkTransferStatus::Completed)
            || status == static_cast<int>(NetworkTransferStatus::Failed)
            || status == static_cast<int>(NetworkTransferStatus::Canceled);
    }
};

#endif // HOMETASKSTATUSPOLICY_H
