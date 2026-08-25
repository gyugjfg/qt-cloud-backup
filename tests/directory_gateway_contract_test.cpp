#include "DirectoryGateway.h"

#include "NetWork.h"

#include <QtTest>

class DirectoryGatewayContractTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 空网络依赖只能返回安全默认值，不能触发空指针调用。 */
    void nullNetworkReturnsSafeReadDefaults();
    /** 未登记节点的目录读取必须保持空列表失败语义。 */
    void unknownNodeReturnsEmptyList();
    /** 当前 fileInfo 契约只生成路径元数据，不伪装成远程 stat。 */
    void fileInfoKeepsPathMetadataOnly();
    /** 空网络依赖下的同步控制是安全空操作。 */
    void nullNetworkSyncIsNoOp();
};

void DirectoryGatewayContractTest::nullNetworkReturnsSafeReadDefaults()
{
    DirectoryGateway gateway(nullptr);

    const QList<DirectoryGateway::FileInfo> fileList = gateway.fileInfoList(
        QStringLiteral("node-1"), QStringLiteral("/"));
    QVERIFY(fileList.isEmpty());

    const DirectoryGateway::FileInfo info = gateway.fileInfo(
        QStringLiteral("node-1"), QStringLiteral("/archive.zip"));
    QVERIFY(info.fileName.isEmpty());
    QVERIFY(info.filePath.isEmpty());
    QCOMPARE(info.fileSize, qint64(0));
    QVERIFY(!info.modifyTime.isValid());
    QVERIFY(!info.isDirectory);
}

void DirectoryGatewayContractTest::unknownNodeReturnsEmptyList()
{
    NetWork network;
    DirectoryGateway gateway(&network);

    const QList<DirectoryGateway::FileInfo> fileList = gateway.fileInfoList(
        QStringLiteral("missing-node"), QStringLiteral("./"));
    QVERIFY(fileList.isEmpty());
}

void DirectoryGatewayContractTest::fileInfoKeepsPathMetadataOnly()
{
    NetWork network;
    DirectoryGateway gateway(&network);
    const QString remotePath = QStringLiteral("./documents/archive.zip");

    const DirectoryGateway::FileInfo info = gateway.fileInfo(
        QStringLiteral("node-1"), remotePath);

    QCOMPARE(info.filePath, remotePath);
    QCOMPARE(info.fileName, QStringLiteral("archive.zip"));
    QCOMPARE(info.fileSize, qint64(0));
    QVERIFY(!info.modifyTime.isValid());
    QVERIFY(!info.isDirectory);
}

void DirectoryGatewayContractTest::nullNetworkSyncIsNoOp()
{
    DirectoryGateway gateway(nullptr);

    gateway.startFileListSync(QStringLiteral("node-1"), 1);
    gateway.stopFileListSync(QStringLiteral("node-1"));
    QVERIFY(true);
}

QTEST_MAIN(DirectoryGatewayContractTest)
#include "directory_gateway_contract_test.moc"
