#include "HomeTaskStatusPolicy.h"

#include <QtTest>

class HomeTaskStatusPolicyTest : public QObject
{
    Q_OBJECT

private slots:
    /** 完成、失败和取消必须被识别为终态。 */
    void terminalStatusesAreRecognized();
    /** 等待、运行和暂停仍允许后续状态回流。 */
    void activeStatusesAreNotTerminal();
    /** 未知整数不应误触发主页完成汇总。 */
    void unknownStatusesAreNotTerminal();
};

void HomeTaskStatusPolicyTest::terminalStatusesAreRecognized()
{
    QVERIFY(HomeTaskStatusPolicy::isTerminal(
        static_cast<int>(NetworkTransferStatus::Completed)));
    QVERIFY(HomeTaskStatusPolicy::isTerminal(
        static_cast<int>(NetworkTransferStatus::Failed)));
    QVERIFY(HomeTaskStatusPolicy::isTerminal(
        static_cast<int>(NetworkTransferStatus::Canceled)));
}

void HomeTaskStatusPolicyTest::activeStatusesAreNotTerminal()
{
    QVERIFY(!HomeTaskStatusPolicy::isTerminal(
        static_cast<int>(NetworkTransferStatus::Waiting)));
    QVERIFY(!HomeTaskStatusPolicy::isTerminal(
        static_cast<int>(NetworkTransferStatus::Running)));
    QVERIFY(!HomeTaskStatusPolicy::isTerminal(
        static_cast<int>(NetworkTransferStatus::Paused)));
}

void HomeTaskStatusPolicyTest::unknownStatusesAreNotTerminal()
{
    QVERIFY(!HomeTaskStatusPolicy::isTerminal(-1));
    QVERIFY(!HomeTaskStatusPolicy::isTerminal(999));
}

QTEST_MAIN(HomeTaskStatusPolicyTest)
#include "home_task_status_policy_test.moc"
