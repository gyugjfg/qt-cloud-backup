#include "DirectoryService.h"
#include "NodeService.h"

#include <QHostAddress>
#include <QElapsedTimer>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtTest>

class ServiceAsyncSlowNetworkTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 停止同步后，挂起请求恢复时不能再发出迟到的目录快照。 */
    void cancellationSuppressesLateDirectoryResult();
    /** 服务析构后，挂起请求恢复时不能访问已销毁服务。 */
    void destructionSuppressesLateDirectoryResult();
};

namespace {
NetworkNodeInfo nodeForServer(const QTcpServer &server)
{
    NetworkNodeInfo node;
    node.nodeId = QStringLiteral("slow-directory-node");
    node.nodeName = QStringLiteral("慢网络目录测试节点");
    node.ip = QStringLiteral("127.0.0.1");
    node.port = static_cast<int>(server.serverPort());
    return node;
}

QTcpSocket *waitForDirectoryRequest(QTcpServer &server, DirectoryService *service)
{
    QTimer *timer = service->findChild<QTimer *>();
    if (!timer || !QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection)) {
        return nullptr;
    }

    QElapsedTimer waitTimer;
    waitTimer.start();
    while (!server.hasPendingConnections() && waitTimer.elapsed() < 3000) {
        QTest::qWait(20);
    }
    if (!server.hasPendingConnections()) {
        return nullptr;
    }
    return server.nextPendingConnection();
}

void releaseDirectoryRequest(QTcpSocket *peer)
{
    QVERIFY(peer);
    peer->write("late.txt|1|0\n");
    QVERIFY(peer->waitForBytesWritten(1000));
    peer->disconnectFromHost();
}
}

void ServiceAsyncSlowNetworkTest::cancellationSuppressesLateDirectoryResult()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    NodeService nodeService;
    QVERIFY(nodeService.addNode(nodeForServer(server)));

    DirectoryService service(&nodeService);
    QSignalSpy updateSpy(&service, &DirectoryService::fileListUpdated);
    service.startFileListSync(QStringLiteral("slow-directory-node"), 1);

    QScopedPointer<QTcpSocket> peer(waitForDirectoryRequest(server, &service));
    QVERIFY(!peer.isNull());

    service.stopFileListSync(QStringLiteral("slow-directory-node"));
    releaseDirectoryRequest(peer.data());

    QTest::qWait(300);
    QCOMPARE(updateSpy.count(), 0);
}

void ServiceAsyncSlowNetworkTest::destructionSuppressesLateDirectoryResult()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    NodeService nodeService;
    QVERIFY(nodeService.addNode(nodeForServer(server)));

    auto *service = new DirectoryService(&nodeService);
    QSignalSpy updateSpy(service, &DirectoryService::fileListUpdated);
    service->startFileListSync(QStringLiteral("slow-directory-node"), 1);

    QScopedPointer<QTcpSocket> peer(waitForDirectoryRequest(server, service));
    QVERIFY(!peer.isNull());

    delete service;
    releaseDirectoryRequest(peer.data());

    QTest::qWait(300);
    QCOMPARE(updateSpy.count(), 0);
}

QTEST_MAIN(ServiceAsyncSlowNetworkTest)
#include "service_async_slow_network_test.moc"
