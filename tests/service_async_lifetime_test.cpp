#include "DirectoryService.h"
#include "NodeService.h"
#include "TransferService.h"

#include <QFile>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>
#include <QTimer>
#include <QtTest>

class ServiceAsyncLifetimeTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 节点探测 worker 已启动后服务销毁，不能再回写节点或发信号。 */
    void nodeStatusWorkerStopsAfterServiceDestruction();
    /** 目录同步 worker 已启动后服务销毁，不能再回写目录快照。 */
    void directorySyncWorkerStopsAfterServiceDestruction();
    /** 活跃上传 socket 被服务析构关闭后，worker 必须收敛而不访问悬空对象。 */
    void activeTransferWorkerStopsAfterServiceDestruction();
};

namespace {
NetworkNodeInfo loopbackNode()
{
    NetworkNodeInfo node;
    node.nodeId = QStringLiteral("lifetime-node");
    node.nodeName = QStringLiteral("生命周期测试节点");
    node.ip = QStringLiteral("127.0.0.1");
    node.port = 1;
    return node;
}
}

void ServiceAsyncLifetimeTest::nodeStatusWorkerStopsAfterServiceDestruction()
{
    auto *service = new NodeService;
    QVERIFY(service->addNode(loopbackNode()));

    service->checkNodeStatusAsync(QStringLiteral("lifetime-node"));
    delete service;

    // 回环端口 1 会快速拒绝连接；等待 queued callback 到期，确保不会访问已销毁对象。
    QTest::qWait(300);
    QVERIFY(true);
}

void ServiceAsyncLifetimeTest::directorySyncWorkerStopsAfterServiceDestruction()
{
    NodeService nodeService;
    QVERIFY(nodeService.addNode(loopbackNode()));

    auto *service = new DirectoryService(&nodeService);
    service->startFileListSync(QStringLiteral("lifetime-node"), 1);

    QTimer *timer = service->findChild<QTimer *>();
    QVERIFY(timer);
    QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));

    delete service;

    // stopFileListSync 已使本次 worker 的共享令牌失效；这里只等待线程池回收。
    QTest::qWait(300);
    QVERIFY(true);
}

void ServiceAsyncLifetimeTest::activeTransferWorkerStopsAfterServiceDestruction()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryFile sourceFile;
    QVERIFY(sourceFile.open());
    const QByteArray payload("lifecycle-test-payload");
    QCOMPARE(sourceFile.write(payload), payload.size());
    sourceFile.flush();

    TransferService::TransferRequest request;
    request.type = TransferService::TransferKind::Upload;
    request.filePath = sourceFile.fileName();
    request.fileName = QStringLiteral("lifecycle-test.bin");
    request.nodeId = QStringLiteral("lifetime-node");
    request.nodeIp = QStringLiteral("127.0.0.1");
    request.nodePort = static_cast<int>(server.serverPort());
    request.taskId = QStringLiteral("lifetime-upload");

    auto *service = new TransferService;
    service->startTransferAsync(request);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);
    QScopedPointer<QTcpSocket> peer(server.nextPendingConnection());

    // 服务端故意不回复 READY，让 worker 停在协议读取阶段，随后由析构关闭 active socket。
    QTest::qWait(50);
    delete service;
    QVERIFY(true);
}

QTEST_MAIN(ServiceAsyncLifetimeTest)
#include "service_async_lifetime_test.moc"
