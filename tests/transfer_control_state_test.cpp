#include "TransferControlState.h"

#include <QtTest>

class TransferControlStateTest final : public QObject
{
    Q_OBJECT

private slots:
    /** 同一任务的最后一个控制请求决定暂停或取消结果。 */
    void lastControlRequestWins();
    /** 上传和下载方向共享 taskId 时仍保持状态隔离。 */
    void directionsRemainIndependent();
    /** 服务关闭优先于普通控制请求，活动 socket 可被统一枚举。 */
    void shutdownAndSocketSnapshot();
};

void TransferControlStateTest::lastControlRequestWins()
{
    TransferControlState state;
    const QString taskId = QStringLiteral("control-task");
    state.registerSocket(TransferControlState::Direction::Download, taskId, 41);

    QCOMPARE(state.request(TransferControlState::Direction::Download,
                           taskId,
                           TransferControlState::Action::Pause),
             41);
    QCOMPARE(state.check(TransferControlState::Direction::Download, taskId, false),
             TransferControlState::Result::Paused);

    state.registerSocket(TransferControlState::Direction::Download, taskId, 42);
    QCOMPARE(state.request(TransferControlState::Direction::Download,
                           taskId,
                           TransferControlState::Action::Cancel),
             42);
    QCOMPARE(state.check(TransferControlState::Direction::Download, taskId, false),
             TransferControlState::Result::Canceled);
}

void TransferControlStateTest::directionsRemainIndependent()
{
    TransferControlState state;
    const QString taskId = QStringLiteral("same-id");
    state.registerSocket(TransferControlState::Direction::Upload, taskId, 51);
    state.registerSocket(TransferControlState::Direction::Download, taskId, 52);

    state.request(TransferControlState::Direction::Upload,
                  taskId,
                  TransferControlState::Action::Pause);
    QCOMPARE(state.check(TransferControlState::Direction::Upload, taskId, false),
             TransferControlState::Result::Paused);
    QCOMPARE(state.check(TransferControlState::Direction::Download, taskId, false),
             TransferControlState::Result::Continue);
    QCOMPARE(state.request(TransferControlState::Direction::Download,
                           taskId,
                           TransferControlState::Action::Cancel),
             52);
}

void TransferControlStateTest::shutdownAndSocketSnapshot()
{
    TransferControlState state;
    state.registerSocket(TransferControlState::Direction::Upload,
                         QStringLiteral("upload"),
                         61);
    state.registerSocket(TransferControlState::Direction::Download,
                         QStringLiteral("download"),
                         62);

    QCOMPARE(state.check(TransferControlState::Direction::Upload,
                         QStringLiteral("upload"),
                         true),
             TransferControlState::Result::Canceled);
    QCOMPARE(state.activeSockets(TransferControlState::Direction::Upload), QList<int>({61}));
    QCOMPARE(state.activeSockets(TransferControlState::Direction::Download), QList<int>({62}));

    state.clear();
    QVERIFY(state.activeSockets(TransferControlState::Direction::Upload).isEmpty());
    QVERIFY(state.activeSockets(TransferControlState::Direction::Download).isEmpty());
}

QTEST_MAIN(TransferControlStateTest)
#include "transfer_control_state_test.moc"
