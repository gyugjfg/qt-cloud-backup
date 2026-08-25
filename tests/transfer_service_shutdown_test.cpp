#include "TransferService.h"

#include <QSignalSpy>
#include <QtTest>

class TransferServiceShutdownTest : public QObject
{
    Q_OBJECT

private slots:
    /** 无效同步请求必须通过统一失败信号回传，而不是启动 socket。 */
    void invalidSynchronousRequestReportsFailure();
    /** 无效异步请求必须由队列执行并回传失败，证明队列没有空壳吞掉请求。 */
    void invalidAsynchronousRequestReportsFailure();
    /** 同一批排队请求必须逐项执行并分别回传失败，不能只处理队首。 */
    void queuedRequestsDrainIndependently();
    /** 服务析构时已投递请求应被安全清理，不留下悬空 worker。 */
    void destructionDrainsQueuedRequest();
};

void TransferServiceShutdownTest::invalidSynchronousRequestReportsFailure()
{
    TransferService service;
    QSignalSpy errorSpy(&service, &TransferService::taskError);
    QSignalSpy statusSpy(&service, &TransferService::taskStatusChanged);

    TransferService::TransferRequest request;
    request.taskId = QStringLiteral("invalid-sync");
    QVERIFY(!service.fileUpload(request));

    QTRY_COMPARE(errorSpy.count(), 1);
    QTRY_COMPARE(statusSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toString(), QStringLiteral("invalid-sync"));
    QCOMPARE(statusSpy.at(0).at(0).toString(), QStringLiteral("invalid-sync"));
    QCOMPARE(statusSpy.at(0).at(1).toInt(), static_cast<int>(NetworkTransferStatus::Failed));
}

void TransferServiceShutdownTest::invalidAsynchronousRequestReportsFailure()
{
    TransferService service;
    QSignalSpy errorSpy(&service, &TransferService::taskError);
    QSignalSpy statusSpy(&service, &TransferService::taskStatusChanged);

    TransferService::TransferRequest request;
    request.taskId = QStringLiteral("invalid-async");
    service.startTransferAsync(request);

    QTRY_COMPARE_WITH_TIMEOUT(errorSpy.count(), 1, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(statusSpy.count(), 1, 3000);
    QCOMPARE(errorSpy.at(0).at(0).toString(), QStringLiteral("invalid-async"));
    QCOMPARE(statusSpy.at(0).at(0).toString(), QStringLiteral("invalid-async"));
    QCOMPARE(statusSpy.at(0).at(1).toInt(), static_cast<int>(NetworkTransferStatus::Failed));
}

void TransferServiceShutdownTest::queuedRequestsDrainIndependently()
{
    TransferService service;
    QSignalSpy errorSpy(&service, &TransferService::taskError);
    QSignalSpy statusSpy(&service, &TransferService::taskStatusChanged);

    TransferService::TransferRequest first;
    first.taskId = QStringLiteral("queued-invalid-first");
    TransferService::TransferRequest second;
    second.taskId = QStringLiteral("queued-invalid-second");

    service.startTransferAsync(first);
    service.startTransferAsync(second);

    QTRY_COMPARE_WITH_TIMEOUT(errorSpy.count(), 2, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(statusSpy.count(), 2, 3000);

    QStringList errorTaskIds;
    for (const QList<QVariant> &arguments : errorSpy) {
        errorTaskIds.append(arguments.value(0).toString());
    }
    QStringList statusTaskIds;
    for (const QList<QVariant> &arguments : statusSpy) {
        statusTaskIds.append(arguments.value(0).toString());
        QCOMPARE(arguments.value(1).toInt(), static_cast<int>(NetworkTransferStatus::Failed));
    }

    QVERIFY(errorTaskIds.contains(first.taskId));
    QVERIFY(errorTaskIds.contains(second.taskId));
    QVERIFY(statusTaskIds.contains(first.taskId));
    QVERIFY(statusTaskIds.contains(second.taskId));
}

void TransferServiceShutdownTest::destructionDrainsQueuedRequest()
{
    auto *service = new TransferService;
    TransferService::TransferRequest request;
    request.taskId = QStringLiteral("shutdown-before-worker");
    service->startTransferAsync(request);

    // 析构必须等待已投递 worker 结束；若 worker 继续访问悬空 this，测试进程会崩溃。
    delete service;
    QVERIFY(true);
}

QTEST_MAIN(TransferServiceShutdownTest)
#include "transfer_service_shutdown_test.moc"
