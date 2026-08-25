#include "TransferTypes.h"

#include <QtTest>

class TransferContractTest : public QObject
{
    Q_OBJECT

private slots:
    /** 状态码是任务层和网络层之间的共享契约，不能无意改序。 */
    void statusValuesRemainStable();
    /** 默认请求必须保持任务创建链当前使用的下载/单线程/零偏移语义。 */
    void requestDefaultsAreExplicit();
};

void TransferContractTest::statusValuesRemainStable()
{
    QCOMPARE(static_cast<int>(NetworkTransferStatus::Waiting), 0);
    QCOMPARE(static_cast<int>(NetworkTransferStatus::Running), 1);
    QCOMPARE(static_cast<int>(NetworkTransferStatus::Paused), 2);
    QCOMPARE(static_cast<int>(NetworkTransferStatus::Completed), 3);
    QCOMPARE(static_cast<int>(NetworkTransferStatus::Failed), 4);
    QCOMPARE(static_cast<int>(NetworkTransferStatus::Canceled), 5);
}

void TransferContractTest::requestDefaultsAreExplicit()
{
    const NetworkTransferRequest request;

    QCOMPARE(request.type, NetworkTransferType::Download);
    QVERIFY(request.filePath.isEmpty());
    QVERIFY(request.fileName.isEmpty());
    QVERIFY(request.savePath.isEmpty());
    QVERIFY(request.nodeId.isEmpty());
    QVERIFY(request.taskId.isEmpty());
    QCOMPARE(request.threadCount, 1);
    QCOMPARE(request.startOffset, qint64(0));
}

QTEST_MAIN(TransferContractTest)
#include "transfer_contract_test.moc"
