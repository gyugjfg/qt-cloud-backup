#include "TransferProtocolClient.h"
#include "TransferService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {

QByteArray testPayload()
{
    QByteArray payload;
    payload.reserve(256 * 1024 + 29);
    for (int index = 0; index < 256 * 1024 + 29; ++index) {
        payload.append(static_cast<char>((index * 17 + 11) % 251));
    }
    return payload;
}

QString requiredEnvironment(const char *name)
{
    return qEnvironmentVariable(name).trimmed();
}

} // namespace

/**
 * @brief Windows Qt 客户端连接 WSL/Linux server.cpp 的真实协议联调。
 *
 * 测试不启动服务端进程，服务端由外部命令提供；这样 server.cpp 的
 * 生命周期和 QtTest 的客户端断言边界清晰分开，失败时也能区分连接问题
 * 与文件内容问题。未配置环境变量时主动跳过，不把未联调写成通过。
 */
class TransferServiceLinuxE2eTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 真实 Linux 服务端应接受上传，并支持按本地前缀继续下载。 */
    void uploadAndResumeDownloadAgainstLinuxServer();
};

void TransferServiceLinuxE2eTest::uploadAndResumeDownloadAgainstLinuxServer()
{
    const QString serverIp = requiredEnvironment("CLOUD_BACKUP_SERVER_IP");
    const QString portText = requiredEnvironment("CLOUD_BACKUP_SERVER_PORT");
    const QString serverRoot = requiredEnvironment("CLOUD_BACKUP_SERVER_ROOT");
    if (serverIp.isEmpty() || portText.isEmpty() || serverRoot.isEmpty()) {
        QSKIP("set CLOUD_BACKUP_SERVER_IP, CLOUD_BACKUP_SERVER_PORT and CLOUD_BACKUP_SERVER_ROOT to run Linux E2E");
    }

    bool portOk = false;
    const int serverPort = portText.toInt(&portOk);
    QVERIFY2(portOk && serverPort > 0 && serverPort <= 65535,
             "CLOUD_BACKUP_SERVER_PORT must be a valid TCP port");
    QVERIFY2(QDir(serverRoot).exists(), "CLOUD_BACKUP_SERVER_ROOT must be a shared existing directory");

    const QByteArray payload = testPayload();
    const QString remoteFileName = QStringLiteral("qt-client-linux-e2e.bin");
    const QString remoteFilePath = QDir(serverRoot).filePath(remoteFileName);
    QFile::remove(remoteFilePath);

    QTemporaryDir clientDir;
    QVERIFY(clientDir.isValid());
    const QString sourcePath = clientDir.filePath(remoteFileName);
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    QCOMPARE(sourceFile.write(payload), payload.size());
    sourceFile.close();

    TransferService service;
    QSignalSpy statusSpy(&service, &TransferService::taskStatusChanged);
    TransferService::TransferRequest upload;
    upload.type = TransferService::TransferKind::Upload;
    upload.filePath = sourcePath;
    upload.fileName = remoteFileName;
    upload.nodeId = QStringLiteral("linux-e2e-node");
    upload.nodeIp = serverIp;
    upload.nodePort = serverPort;
    upload.taskId = QStringLiteral("linux-e2e-upload");
    QVERIFY2(service.fileUpload(upload), "TransferService upload to Linux server failed");

    QFileInfo remoteInfo(remoteFilePath);
    QVERIFY2(remoteInfo.exists(), "Linux server did not create the uploaded file in shared root");
    QCOMPARE(remoteInfo.size(), qint64(payload.size()));

    const qint64 prefixSize = payload.size() / 3;
    const QString downloadPath = clientDir.filePath(QStringLiteral("resumed-download.bin"));
    QFile partialFile(downloadPath);
    QVERIFY(partialFile.open(QIODevice::WriteOnly));
    QCOMPARE(partialFile.write(payload.left(prefixSize)), prefixSize);
    partialFile.close();

    TransferService::TransferRequest download;
    download.type = TransferService::TransferKind::Download;
    download.fileName = remoteFileName;
    download.savePath = downloadPath;
    download.nodeId = QStringLiteral("linux-e2e-node");
    download.nodeIp = serverIp;
    download.nodePort = serverPort;
    download.taskId = QStringLiteral("linux-e2e-download");
    download.startOffset = prefixSize;
    QVERIFY2(service.fileDownload(download), "TransferService resumed download from Linux server failed");

    QFile downloadedFile(downloadPath);
    QVERIFY(downloadedFile.open(QIODevice::ReadOnly));
    QCOMPARE(downloadedFile.readAll(), payload);
    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.count() >= 2, 2000);

    QVERIFY2(QFile::remove(remoteFilePath), "failed to clean Linux E2E remote file");
}

QTEST_MAIN(TransferServiceLinuxE2eTest)
#include "transfer_service_linux_e2e_test.moc"
