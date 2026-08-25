#include "DirectoryLoadCoordinator.h"

#include <QtTest>

#include <QThread>

class DirectoryLoadCoordinatorTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 正常请求只在主线程回调一次，并保留值对象内容。 */
    void deliversValueSnapshotOnGuiThread();
    /** 新请求会淘汰慢请求，旧结果不得回调。 */
    void dropsStaleSlowRequest();
    /** 协调器销毁后，后台请求不得回写悬空调用方。 */
    void destructionDropsPendingCallback();
};

void DirectoryLoadCoordinatorTest::deliversValueSnapshotOnGuiThread()
{
    DirectoryLoadCoordinator coordinator([](const QString &, const QString &path) {
        NetworkFileInfo info;
        info.fileName = QStringLiteral("report.txt");
        info.filePath = path + QStringLiteral("/report.txt");
        return QList<NetworkFileInfo>{info};
    });

    int callbackCount = 0;
    Qt::HANDLE callbackThread = nullptr;
    QString callbackPath;
    coordinator.request(QStringLiteral("node-1"), QStringLiteral("/documents"),
                         [&](quint64, const QString &, const QString &path,
                             const QList<NetworkFileInfo> &fileList) {
        ++callbackCount;
        callbackThread = QThread::currentThreadId();
        callbackPath = path;
        QCOMPARE(fileList.size(), 1);
        QCOMPARE(fileList.first().filePath, QStringLiteral("/documents/report.txt"));
    });

    QTRY_COMPARE_WITH_TIMEOUT(callbackCount, 1, 3000);
    QCOMPARE(callbackThread, QThread::currentThreadId());
    QCOMPARE(callbackPath, QStringLiteral("/documents"));
}

void DirectoryLoadCoordinatorTest::dropsStaleSlowRequest()
{
    DirectoryLoadCoordinator coordinator([](const QString &, const QString &path) {
        if (path == QStringLiteral("/slow")) {
            QThread::msleep(160);
        }
        NetworkFileInfo info;
        info.filePath = path;
        return QList<NetworkFileInfo>{info};
    });

    int callbackCount = 0;
    QString callbackPath;
    coordinator.request(QStringLiteral("node-1"), QStringLiteral("/slow"),
                         [&](quint64, const QString &, const QString &path,
                             const QList<NetworkFileInfo> &) {
        ++callbackCount;
        callbackPath = path;
    });
    QTest::qWait(20);
    coordinator.request(QStringLiteral("node-1"), QStringLiteral("/fast"),
                         [&](quint64, const QString &, const QString &path,
                             const QList<NetworkFileInfo> &) {
        ++callbackCount;
        callbackPath = path;
    });

    QTRY_COMPARE_WITH_TIMEOUT(callbackCount, 1, 3000);
    QCOMPARE(callbackPath, QStringLiteral("/fast"));
    QTest::qWait(220);
    QCOMPARE(callbackCount, 1);
}

void DirectoryLoadCoordinatorTest::destructionDropsPendingCallback()
{
    int callbackCount = 0;
    auto *coordinator = new DirectoryLoadCoordinator([](const QString &, const QString &) {
        QThread::msleep(160);
        return QList<NetworkFileInfo>();
    });
    coordinator->request(QStringLiteral("node-1"), QStringLiteral("/slow"),
                         [&](quint64, const QString &, const QString &,
                             const QList<NetworkFileInfo> &) {
        ++callbackCount;
    });
    delete coordinator;

    QTest::qWait(260);
    QCOMPARE(callbackCount, 0);
}

QTEST_MAIN(DirectoryLoadCoordinatorTest)
#include "directory_load_coordinator_test.moc"
