#include "TransferRequestPolicy.h"

#include <QtTest>

/**
 * @brief TransferService 请求前置规则的纯值测试。
 *
 * 测试不启动线程、socket 或文件，专门锁定 TransferService 当前使用的端点和
 * 断点偏移语义；真实协议执行链由 transfer_service_e2e_test 继续覆盖。
 */
class TransferRequestPolicyTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 节点标识、地址和端口缺一项时必须拒绝请求。 */
    void endpointRequiresAllConnectionFields();
    /** 上传偏移必须被限制在 [0, fileSize]。 */
    void uploadOffsetIsClamped();
    /** 下载偏移以本地实际长度为准，超长本地文件从头恢复。 */
    void downloadOffsetUsesLocalFileLength();
};

void TransferRequestPolicyTest::endpointRequiresAllConnectionFields()
{
    QVERIFY(!TransferRequestPolicy::hasUsableNodeEndpoint({}, QStringLiteral("127.0.0.1"), 9000));
    QVERIFY(!TransferRequestPolicy::hasUsableNodeEndpoint(QStringLiteral("node"), {}, 9000));
    QVERIFY(!TransferRequestPolicy::hasUsableNodeEndpoint(QStringLiteral("node"),
                                                           QStringLiteral("127.0.0.1"),
                                                           0));
    QVERIFY(!TransferRequestPolicy::hasUsableNodeEndpoint(QStringLiteral("node"),
                                                           QStringLiteral("127.0.0.1"),
                                                           -1));
    QVERIFY(TransferRequestPolicy::hasUsableNodeEndpoint(QStringLiteral("node"),
                                                         QStringLiteral("127.0.0.1"),
                                                         9000));
}

void TransferRequestPolicyTest::uploadOffsetIsClamped()
{
    QCOMPARE(TransferRequestPolicy::clampUploadOffset(-1, 100), qint64(0));
    QCOMPARE(TransferRequestPolicy::clampUploadOffset(40, 100), qint64(40));
    QCOMPARE(TransferRequestPolicy::clampUploadOffset(1000, 100), qint64(100));
    QCOMPARE(TransferRequestPolicy::clampUploadOffset(3, 0), qint64(0));
}

void TransferRequestPolicyTest::downloadOffsetUsesLocalFileLength()
{
    QCOMPARE(TransferRequestPolicy::resolveDownloadOffset(0, 100), qint64(0));
    QCOMPARE(TransferRequestPolicy::resolveDownloadOffset(40, 100), qint64(40));
    QCOMPARE(TransferRequestPolicy::resolveDownloadOffset(100, 100), qint64(100));
    QCOMPARE(TransferRequestPolicy::resolveDownloadOffset(120, 100), qint64(0));
}

QTEST_MAIN(TransferRequestPolicyTest)
#include "transfer_request_policy_test.moc"
