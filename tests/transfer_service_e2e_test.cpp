#include "TransferService.h"
#include "TransferProtocolClient.h"

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QHostAddress>
#include <QSignalSpy>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QtTest>

/**
 * @brief 进程内 fileput/filesave 协议 fixture。
 *
 * fixture 使用 QTcpServer 模拟当前 server.cpp 的握手顺序，测试仍调用
 * TransferService 的真实 native socket 客户端和本地文件落盘逻辑；
 * 它不替代 Linux server smoke，也不声称覆盖真实跨平台部署。
 */
class TransferProtocolFixture final : public QObject
{
    Q_OBJECT

public:
    explicit TransferProtocolFixture(QByteArray remotePayload,
                                     bool slowDownload = false,
                                     QObject *parent = nullptr)
        : QObject(parent)
        , m_remotePayload(std::move(remotePayload))
        , m_slowDownload(slowDownload)
    {
        m_server.setParent(this);
    }

    quint16 port() const { return m_server.serverPort(); }
    QByteArray uploadedPayload() const { return m_uploadedPayload; }

public slots:
    void start()
    {
        connect(&m_server, &QTcpServer::newConnection,
                this, &TransferProtocolFixture::acceptConnections);
        QVERIFY(m_server.listen(QHostAddress::LocalHost, 0));
        emit ready();
    }

    void stop()
    {
        const auto clients = m_states.keys();
        for (QTcpSocket *client : clients) {
            client->disconnectFromHost();
            client->deleteLater();
        }
        m_states.clear();
        m_server.close();
    }

    void setSlowDownload(bool slowDownload) { m_slowDownload = slowDownload; }

signals:
    void ready();
    /** 已收到区间下载请求，测试可在此确认慢网络已建立。 */
    void rangeRequestReady();

private slots:
    void acceptConnections()
    {
        while (m_server.hasPendingConnections()) {
            QTcpSocket *client = m_server.nextPendingConnection();
            m_states.insert(client, ClientState{});
            connect(client, &QTcpSocket::readyRead,
                    this, &TransferProtocolFixture::readClient);
            connect(client, &QTcpSocket::disconnected,
                    this, &TransferProtocolFixture::removeClient);
        }
    }

    void removeClient()
    {
        auto *client = qobject_cast<QTcpSocket *>(sender());
        if (!client) {
            return;
        }
        m_states.remove(client);
        client->deleteLater();
    }

    void readClient()
    {
        auto *client = qobject_cast<QTcpSocket *>(sender());
        if (!client || !m_states.contains(client)) {
            return;
        }

        ClientState &state = m_states[client];
        state.buffer += client->readAll();
        if (state.mode == ClientMode::Unknown) {
            const int lineEnd = state.buffer.indexOf('\n');
            if (lineEnd < 0) {
                return;
            }

            const QByteArray command = state.buffer.left(lineEnd).trimmed();
            state.buffer.remove(0, lineEnd + 1);
            const QList<QByteArray> fields = command.split('|');
            if (fields.size() == 5 && fields.at(0) == "fileput") {
                state.mode = ClientMode::Upload;
                state.expectedSize = fields.at(3).toLongLong();
                client->write("READY:0\n");
                client->flush();
            } else if (fields.size() == 2 && fields.at(0) == "filesave") {
                client->write(QByteArray::number(m_remotePayload.size()) + "\n");
                client->flush();
                client->disconnectFromHost();
                return;
            } else if (fields.size() == 4 && fields.at(0) == "filesave") {
                state.mode = ClientMode::DownloadRange;
                state.startOffset = fields.at(2).toLongLong();
                state.endOffset = fields.at(3).toLongLong();
                client->write(QByteArray::number(m_remotePayload.size()) + "\n");
                client->flush();
                emit rangeRequestReady();
            } else {
                client->write("ERROR: fixture command\n");
                client->disconnectFromHost();
                return;
            }
        }

        if (state.mode == ClientMode::Upload) {
            state.payload += state.buffer;
            state.buffer.clear();
            if (state.payload.size() >= state.expectedSize) {
                m_uploadedPayload = state.payload.left(state.expectedSize);
                client->write("OK:" + QByteArray::number(state.expectedSize) + "\n");
                client->flush();
                client->disconnectFromHost();
            }
            return;
        }

        if (state.mode == ClientMode::DownloadRange) {
            const int lineEnd = state.buffer.indexOf('\n');
            if (lineEnd < 0) {
                return;
            }
            const QByteArray confirmation = state.buffer.left(lineEnd).trimmed();
            if (confirmation != "OK") {
                client->disconnectFromHost();
                return;
            }

            const qint64 start = qBound<qint64>(0, state.startOffset, m_remotePayload.size());
            const qint64 end = qBound<qint64>(-1, state.endOffset, m_remotePayload.size() - 1);
            if (start <= end) {
                if (m_slowDownload) {
                    const QByteArray firstChunk = m_remotePayload.mid(start,
                                                                       qMin<qint64>(1024, end - start + 1));
                    client->write(firstChunk);
                    client->flush();
                    const QPointer<QTcpSocket> guardedClient(client);
                    QTimer::singleShot(1500, this, [this, guardedClient, start, end]() {
                        if (!guardedClient || !m_states.contains(guardedClient)) {
                            return;
                        }
                        const qint64 firstSize = qMin<qint64>(1024, end - start + 1);
                        guardedClient->write(m_remotePayload.mid(start + firstSize,
                                                                 end - start + 1 - firstSize));
                        guardedClient->flush();
                        guardedClient->disconnectFromHost();
                    });
                    return;
                }
                client->write(m_remotePayload.mid(start, end - start + 1));
                client->flush();
            }
            client->disconnectFromHost();
        }
    }

private:
    enum class ClientMode {
        Unknown,
        Upload,
        DownloadRange
    };

    struct ClientState {
        ClientMode mode = ClientMode::Unknown;
        QByteArray buffer;
        QByteArray payload;
        qint64 expectedSize = 0;
        qint64 startOffset = 0;
        qint64 endOffset = -1;
    };

    QTcpServer m_server;
    QHash<QTcpSocket *, ClientState> m_states;
    QByteArray m_remotePayload;
    QByteArray m_uploadedPayload;
    bool m_slowDownload = false;
};

class TransferServiceE2eTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 通过真实 TransferService native socket 完成上传和断点下载。 */
    void uploadAndDownloadRoundTrip();
    /** 慢 fixture 阻塞接收期间，取消必须回流 Canceled 并收敛 worker。 */
    void cancelSlowDownload();
    /** 暂停应保留部分落盘文件，随后按同一路径恢复并完成。 */
    void pauseAndResumeSlowDownload();
    /** 协议边界必须保留既有命令字段和握手响应格式。 */
    void protocolCommandsKeepLegacyFields();
};

namespace {
QByteArray testPayload()
{
    QByteArray payload;
    payload.reserve(4 * 1024 * 1024 + 17);
    for (int i = 0; i < 4 * 1024 * 1024 + 17; ++i) {
        payload.append(static_cast<char>((i * 31 + 7) % 251));
    }
    return payload;
}

TransferService::TransferRequest uploadRequest(const QString &sourcePath, quint16 port)
{
    TransferService::TransferRequest request;
    request.type = TransferService::TransferKind::Upload;
    request.filePath = sourcePath;
    request.fileName = QStringLiteral("fixture-upload.bin");
    request.nodeId = QStringLiteral("fixture-node");
    request.nodeIp = QStringLiteral("127.0.0.1");
    request.nodePort = static_cast<int>(port);
    request.taskId = QStringLiteral("fixture-upload-task");
    return request;
}
}

void TransferServiceE2eTest::uploadAndDownloadRoundTrip()
{
    const QByteArray payload = testPayload();
    TransferProtocolFixture fixture(payload);
    QThread fixtureThread;
    fixture.moveToThread(&fixtureThread);
    connect(&fixtureThread, &QThread::started, &fixture, &TransferProtocolFixture::start);
    fixtureThread.start();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.port() != 0, 3000);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath = tempDir.filePath(QStringLiteral("source.bin"));
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write(payload), payload.size());
    source.close();

    TransferService service;
    QSignalSpy statusSpy(&service, &TransferService::taskStatusChanged);
    QVERIFY(service.fileUpload(uploadRequest(sourcePath, fixture.port())));

    QByteArray uploaded;
    QMetaObject::invokeMethod(&fixture, [&fixture, &uploaded]() {
        uploaded = fixture.uploadedPayload();
    }, Qt::BlockingQueuedConnection);
    QCOMPARE(uploaded, payload);

    const QString targetPath = tempDir.filePath(QStringLiteral("download.bin"));
    TransferService::TransferRequest download;
    download.type = TransferService::TransferKind::Download;
    download.fileName = QStringLiteral("fixture-upload.bin");
    download.savePath = targetPath;
    download.nodeId = QStringLiteral("fixture-node");
    download.nodeIp = QStringLiteral("127.0.0.1");
    download.nodePort = static_cast<int>(fixture.port());
    download.taskId = QStringLiteral("fixture-download-task");
    QVERIFY(service.fileDownload(download));

    QFile downloaded(targetPath);
    QVERIFY(downloaded.open(QIODevice::ReadOnly));
    QCOMPARE(downloaded.readAll(), payload);
    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.count() >= 2, 1000);

    QMetaObject::invokeMethod(&fixture, &TransferProtocolFixture::stop, Qt::BlockingQueuedConnection);
    fixtureThread.quit();
    QVERIFY(fixtureThread.wait(3000));
}

void TransferServiceE2eTest::cancelSlowDownload()
{
    TransferProtocolFixture fixture(testPayload(), true);
    QThread fixtureThread;
    fixture.moveToThread(&fixtureThread);
    connect(&fixtureThread, &QThread::started, &fixture, &TransferProtocolFixture::start);
    fixtureThread.start();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.port() != 0, 3000);

    QSignalSpy rangeSpy(&fixture, &TransferProtocolFixture::rangeRequestReady);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TransferService service;
    QSignalSpy statusSpy(&service, &TransferService::taskStatusChanged);
    TransferService::TransferRequest request;
    request.type = TransferService::TransferKind::Download;
    request.fileName = QStringLiteral("fixture-slow.bin");
    request.savePath = tempDir.filePath(QStringLiteral("slow-download.bin"));
    request.nodeId = QStringLiteral("fixture-node");
    request.nodeIp = QStringLiteral("127.0.0.1");
    request.nodePort = static_cast<int>(fixture.port());
    request.taskId = QStringLiteral("fixture-slow-download-task");
    service.startTransferAsync(request);

    QTRY_VERIFY_WITH_TIMEOUT(rangeSpy.count() > 0, 3000);
    QTRY_VERIFY_WITH_TIMEOUT([&statusSpy]() {
        for (const QList<QVariant> &arguments : statusSpy) {
            if (arguments.value(1).toInt() == static_cast<int>(NetworkTransferStatus::Running)) {
                return true;
            }
        }
        return false;
    }(), 3000);
    QTest::qWait(50);
    service.controlTransfer(request.taskId,
                            TransferService::TransferKind::Download,
                            TransferService::TransferControlAction::Cancel);

    QTRY_VERIFY_WITH_TIMEOUT([&statusSpy]() {
        for (const QList<QVariant> &arguments : statusSpy) {
            if (arguments.value(1).toInt() == static_cast<int>(NetworkTransferStatus::Canceled)) {
                return true;
            }
        }
        return false;
    }(), 3000);

    QMetaObject::invokeMethod(&fixture, &TransferProtocolFixture::stop, Qt::BlockingQueuedConnection);
    fixtureThread.quit();
    QVERIFY(fixtureThread.wait(3000));
}

void TransferServiceE2eTest::pauseAndResumeSlowDownload()
{
    const QByteArray payload = testPayload();
    TransferProtocolFixture fixture(payload, true);
    QThread fixtureThread;
    fixture.moveToThread(&fixtureThread);
    connect(&fixtureThread, &QThread::started, &fixture, &TransferProtocolFixture::start);
    fixtureThread.start();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.port() != 0, 3000);

    QSignalSpy rangeSpy(&fixture, &TransferProtocolFixture::rangeRequestReady);
    auto *service = new TransferService;
    QSignalSpy statusSpy(service, &TransferService::taskStatusChanged);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TransferService::TransferRequest request;
    request.type = TransferService::TransferKind::Download;
    request.fileName = QStringLiteral("fixture-pause-resume.bin");
    request.savePath = tempDir.filePath(QStringLiteral("pause-resume.bin"));
    request.nodeId = QStringLiteral("fixture-node");
    request.nodeIp = QStringLiteral("127.0.0.1");
    request.nodePort = static_cast<int>(fixture.port());
    request.taskId = QStringLiteral("fixture-pause-resume-task");
    service->startTransferAsync(request);

    QTRY_VERIFY_WITH_TIMEOUT(rangeSpy.count() > 0, 3000);
    QTRY_VERIFY_WITH_TIMEOUT([&statusSpy]() {
        for (const QList<QVariant> &arguments : statusSpy) {
            if (arguments.value(1).toInt() == static_cast<int>(NetworkTransferStatus::Running)) {
                return true;
            }
        }
        return false;
    }(), 3000);
    service->controlTransfer(request.taskId,
                             TransferService::TransferKind::Download,
                             TransferService::TransferControlAction::Pause);

    QTRY_VERIFY_WITH_TIMEOUT([&statusSpy]() {
        for (const QList<QVariant> &arguments : statusSpy) {
            if (arguments.value(1).toInt() == static_cast<int>(NetworkTransferStatus::Paused)) {
                return true;
            }
        }
        return false;
    }(), 3000);

    QFile partialFile(request.savePath);
    QVERIFY(partialFile.open(QIODevice::ReadOnly));
    const qint64 partialSize = partialFile.size();
    QVERIFY(partialSize > 0);
    QVERIFY(partialSize < payload.size());
    partialFile.close();

    QMetaObject::invokeMethod(&fixture, [&fixture]() {
        fixture.setSlowDownload(false);
    }, Qt::BlockingQueuedConnection);
    QVERIFY(service->fileDownload(request));

    QFile resumedFile(request.savePath);
    QVERIFY(resumedFile.open(QIODevice::ReadOnly));
    QCOMPARE(resumedFile.readAll(), payload);
    QTRY_VERIFY_WITH_TIMEOUT([&statusSpy]() {
        for (const QList<QVariant> &arguments : statusSpy) {
            if (arguments.value(1).toInt() == static_cast<int>(NetworkTransferStatus::Completed)) {
                return true;
            }
        }
        return false;
    }(), 1000);

    delete service;
    QMetaObject::invokeMethod(&fixture, &TransferProtocolFixture::stop, Qt::BlockingQueuedConnection);
    fixtureThread.quit();
    QVERIFY(fixtureThread.wait(3000));
}

void TransferServiceE2eTest::protocolCommandsKeepLegacyFields()
{
    QCOMPARE(TransferProtocolClient::uploadCommand(QStringLiteral("sample.bin"), 42, 7),
             QByteArray("fileput|./|sample.bin|42|7\n"));
    QCOMPARE(TransferProtocolClient::downloadSizeCommand(QStringLiteral("sample.bin")),
             QByteArray("filesave|sample.bin\n"));
    QCOMPARE(TransferProtocolClient::downloadRangeCommand(QStringLiteral("sample.bin"), 7, 41),
             QByteArray("filesave|sample.bin|7|41\n"));

    qint64 offset = -1;
    QVERIFY(TransferProtocolClient::parseReadyOffset("READY:7", offset));
    QCOMPARE(offset, qint64(7));
    QVERIFY(TransferProtocolClient::parseOkOffset("OK:42", offset));
    QCOMPARE(offset, qint64(42));
}

QTEST_MAIN(TransferServiceE2eTest)
#include "transfer_service_e2e_test.moc"
