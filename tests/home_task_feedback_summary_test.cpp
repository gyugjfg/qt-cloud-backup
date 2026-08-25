#include "TaskFeedbackSummary.h"

#include <QtTest>

class HomeTaskFeedbackSummaryTest : public QObject
{
    Q_OBJECT

private slots:
    /** 统计应按上传/下载和成功/失败/取消分类，同时保留文件分组顺序。 */
    void aggregatesOutcomesAndBuildsMessage();
    /** 空批次不应伪造弹窗内容。 */
    void emptySummaryHasNoMessage();
};

void HomeTaskFeedbackSummaryTest::aggregatesOutcomesAndBuildsMessage()
{
    TaskFeedbackSummary summary;
    summary.record(QStringLiteral("z.txt"), QString::fromUtf8(u8"\u8282\u70b9\u7532"),
                   TaskFeedbackSummary::Kind::Download,
                   TaskFeedbackSummary::Outcome::Success);
    summary.record(QStringLiteral("a.txt"), QString::fromUtf8(u8"\u8282\u70b9\u4e59"),
                   TaskFeedbackSummary::Kind::Upload,
                   TaskFeedbackSummary::Outcome::Failed);
    summary.record(QStringLiteral("z.txt"), QString::fromUtf8(u8"\u8282\u70b9\u4e19"),
                   TaskFeedbackSummary::Kind::Upload,
                   TaskFeedbackSummary::Outcome::Canceled);

    QCOMPARE(summary.total(), 3);
    QVERIFY(summary.hasRecords());
    const QString text = summary.message();
    QVERIFY(text.toUtf8().contains(QByteArray::fromHex("e4b88ae4bca0")));
    QVERIFY(text.toUtf8().contains(QByteArray::fromHex("e5a4b1e8b4a5")));
    QVERIFY(text.toUtf8().contains(QByteArray::fromHex("e58f96e6b688")));
    QVERIFY(text.contains(QStringLiteral("1. a.txt")));
    QVERIFY(text.contains(QStringLiteral("2. z.txt")));
    QVERIFY(text.contains(QString::fromUtf8(u8"\u4e0a\u4f20 - \u8282\u70b9\u4e19 (\u5df2\u53d6\u6d88)")));
}

void HomeTaskFeedbackSummaryTest::emptySummaryHasNoMessage()
{
    TaskFeedbackSummary summary;
    QVERIFY(!summary.hasRecords());
    QCOMPARE(summary.total(), 0);
    QVERIFY(summary.message().isEmpty());
}

QTEST_MAIN(HomeTaskFeedbackSummaryTest)
#include "home_task_feedback_summary_test.moc"
