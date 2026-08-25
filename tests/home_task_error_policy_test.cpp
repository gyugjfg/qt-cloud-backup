#include "HomeTaskErrorPolicy.h"

#include <QtTest>

class HomeTaskErrorPolicyTest : public QObject
{
    Q_OBJECT

private slots:
    /** upload_ 前缀仍应显示上传错误标题。 */
    void uploadTaskUsesUploadTitle();
    /** 非 upload_ 任务仍按旧规则归入下载错误标题。 */
    void otherTaskUsesDownloadTitle();
    /** 去重键应包含标题和完整错误文本。 */
    void deduplicationKeyKeepsTitleAndMessage();
};

void HomeTaskErrorPolicyTest::uploadTaskUsesUploadTitle()
{
    QCOMPARE(HomeTaskErrorPolicy::titleForTask(QStringLiteral("upload_42")),
             QStringLiteral("上传错误"));
}

void HomeTaskErrorPolicyTest::otherTaskUsesDownloadTitle()
{
    QCOMPARE(HomeTaskErrorPolicy::titleForTask(QStringLiteral("download_42")),
             QStringLiteral("下载错误"));
    QCOMPARE(HomeTaskErrorPolicy::titleForTask(QString()),
             QStringLiteral("下载错误"));
}

void HomeTaskErrorPolicyTest::deduplicationKeyKeepsTitleAndMessage()
{
    QCOMPARE(HomeTaskErrorPolicy::deduplicationKey(
                 QStringLiteral("upload_42"), QStringLiteral("连接失败")),
             QStringLiteral("上传错误|连接失败"));
    QCOMPARE(HomeTaskErrorPolicy::deduplicationKey(
                 QStringLiteral("download_42"), QStringLiteral("连接失败")),
             QStringLiteral("下载错误|连接失败"));
}

QTEST_MAIN(HomeTaskErrorPolicyTest)
#include "home_task_error_policy_test.moc"
