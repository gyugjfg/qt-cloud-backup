#include "TaskPresentationPolicy.h"

#include <QtTest>

class TaskPresentationPolicyTest : public QObject
{
    Q_OBJECT

private slots:
    /** 活动态文案应按状态和上传/下载方向保持旧映射。 */
    void activeStatusTextKeepsDirection();
    /** 已完成列表只允许原有终态文案映射。 */
    void finishedStatusTextKeepsTerminalMapping();
    /** 文件大小阈值和精度应与任务页原实现一致。 */
    void fileSizeFormattingKeepsUnits();
    /** 进度混合文本应只提取最后一个大小部分。 */
    void progressDisplayExtractsSize();
};

void TaskPresentationPolicyTest::activeStatusTextKeepsDirection()
{
    QCOMPARE(TaskPresentationPolicy::taskStatusText(
                 static_cast<int>(NetworkTransferStatus::Waiting), true),
             QString::fromUtf8(u8"\u7b49\u5f85\u4e2d"));
    QCOMPARE(TaskPresentationPolicy::taskStatusText(
                 static_cast<int>(NetworkTransferStatus::Running), true),
             QString::fromUtf8(u8"\u4e0b\u8f7d\u4e2d"));
    QCOMPARE(TaskPresentationPolicy::taskStatusText(
                 static_cast<int>(NetworkTransferStatus::Running), false),
             QString::fromUtf8(u8"\u4e0a\u4f20\u4e2d"));
    QCOMPARE(TaskPresentationPolicy::taskStatusText(
                 static_cast<int>(NetworkTransferStatus::Completed), true),
             QString::fromUtf8(u8"\u5df2\u4e0b\u8f7d"));
    QCOMPARE(TaskPresentationPolicy::taskStatusText(99, false),
             QString::fromUtf8(u8"\u672a\u77e5"));
}

void TaskPresentationPolicyTest::finishedStatusTextKeepsTerminalMapping()
{
    QCOMPARE(TaskPresentationPolicy::finishedStatusText(
                 static_cast<int>(NetworkTransferStatus::Completed), false),
             QString::fromUtf8(u8"\u5df2\u4e0a\u4f20"));
    QCOMPARE(TaskPresentationPolicy::finishedStatusText(
                 static_cast<int>(NetworkTransferStatus::Paused), true),
             QString::fromUtf8(u8"\u5df2\u6682\u505c"));
    QCOMPARE(TaskPresentationPolicy::finishedStatusText(
                 static_cast<int>(NetworkTransferStatus::Canceled), true),
             QString::fromUtf8(u8"\u5df2\u53d6\u6d88"));
    QCOMPARE(TaskPresentationPolicy::finishedStatusText(99, true),
             QString::fromUtf8(u8"\u5931\u8d25"));
}

void TaskPresentationPolicyTest::fileSizeFormattingKeepsUnits()
{
    QCOMPARE(TaskPresentationPolicy::formatFileSize(0),
             QString::fromUtf8(u8"\u672a\u77e5"));
    QCOMPARE(TaskPresentationPolicy::formatFileSize(1023), QStringLiteral("1023 B"));
    QCOMPARE(TaskPresentationPolicy::formatFileSize(1024), QStringLiteral("1.00 KB"));
    QCOMPARE(TaskPresentationPolicy::formatFileSize(1024 * 1024), QStringLiteral("1.00 MB"));
    QCOMPARE(TaskPresentationPolicy::formatFileSize(1024LL * 1024 * 1024),
             QStringLiteral("1.00 GB"));
}

void TaskPresentationPolicyTest::progressDisplayExtractsSize()
{
    QCOMPARE(TaskPresentationPolicy::sizeFromProgressDisplay(
                 QStringLiteral("1.20 MB/s  |  4.00 MB / 8.00 MB")),
             QStringLiteral("8.00 MB"));
    QVERIFY(TaskPresentationPolicy::sizeFromProgressDisplay(
                QString::fromUtf8(u8"\u5931\u8d25")).isEmpty());
    QVERIFY(TaskPresentationPolicy::sizeFromProgressDisplay(QStringLiteral("--")).isEmpty());
}

QTEST_MAIN(TaskPresentationPolicyTest)
#include "task_presentation_policy_test.moc"
