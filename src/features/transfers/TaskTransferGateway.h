#ifndef TASKTRANSFERGATEWAY_H
#define TASKTRANSFERGATEWAY_H

#include "TransferTypes.h"

#include <QObject>

/**
 * @brief 任务层使用的传输窄端口。
 *
 * Gateway 将任务请求转发给 NetWork，并原样转发进度、状态和错误信号；
 * 不维护任务状态、不拥有 NetWork。startTransfer/controlTransfer 均为
 * 非阻塞下发入口，具体结果通过信号异步回到 TaskManager 所在线程。
 */
class NetWork;

class TaskTransferGateway : public QObject
{
    Q_OBJECT

public:
    using TransferRequest = NetworkTransferRequest;
    using TransferType = NetworkTransferType;
    using TransferControlAction = NetworkTransferControlAction;

    /** 注入网络适配器；指针由组合根保证生命周期，Gateway 不负责释放。 */
    explicit TaskTransferGateway(NetWork *network, QObject *parent = nullptr);

    /** 返回网络传输依赖是否已注入。 */
    bool isAvailable() const;
    /** 下发异步传输请求，不阻塞当前调用者；virtual 便于任务层使用进程内测试替身。 */
    virtual void startTransfer(const TransferRequest &request);
    /** 下发暂停或取消意图，具体状态回流仍由任务层处理；virtual 便于验证动作映射。 */
    virtual void controlTransfer(const QString &taskId,
                                 TransferType type,
                                 TransferControlAction action);

signals:
    /** 转发底层传输进度；参数语义与 NetworkTypes 中的请求回流保持一致。 */
    void taskProgressChanged(const QString &taskId, int progress,
                             qint64 transferredSize, qint64 totalSize, double speed);
    /** 转发底层传输状态码。 */
    void taskStatusChanged(const QString &taskId, int status);
    /** 转发底层传输错误文本。 */
    void taskError(const QString &taskId, const QString &errorMessage);

private:
    NetWork *m_network; ///< 借用的网络适配器。
};

#endif // TASKTRANSFERGATEWAY_H
