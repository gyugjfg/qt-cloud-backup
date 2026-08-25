#include "TaskManager.h"

#include <QtTest>

class FakeTaskTransferGateway final : public TaskTransferGateway
{
public:
    struct ControlCall {
        QString taskId;
        TransferType type = TransferType::Download;
        TransferControlAction action = TransferControlAction::Pause;
    };

    FakeTaskTransferGateway()
        : TaskTransferGateway(nullptr)
    {
    }

    void startTransfer(const TransferRequest &request) override
    {
        startedRequests.append(request);
    }

    void controlTransfer(const QString &taskId,
                         TransferType type,
                         TransferControlAction action) override
    {
        controlCalls.append({taskId, type, action});
    }

    QList<TransferRequest> startedRequests;
    QList<ControlCall> controlCalls;
};

class TaskManagerStateTest : public QObject
{
    Q_OBJECT

private slots:
    /** 状态表只允许当前任务生命周期中的相邻迁移。 */
    void transitionTableRemainsStable();
    /** 未注入 Gateway 时，任务层仍能建立本地运行态。 */
    void startTaskKeepsLocalStateWithoutGateway();
    /** 终态和未知任务不能被原子状态更新接口再次改写。 */
    void terminalAndUnknownTasksRejectUpdates();
    /** 任务层必须向 Gateway 发送稳定的请求 DTO 和控制动作。 */
    void gatewayReceivesTransferRequests();
};

void TaskManagerStateTest::transitionTableRemainsStable()
{
    using Status = TaskManager::TaskStatus;

    const struct TransitionCase {
        Status from;
        Status to;
        bool expected;
    } cases[] = {
        {Status::Waiting, Status::Running, true},
        {Status::Waiting, Status::Paused, false},
        {Status::Running, Status::Paused, true},
        {Status::Running, Status::Completed, true},
        {Status::Running, Status::Failed, true},
        {Status::Running, Status::Canceled, true},
        {Status::Paused, Status::Running, true},
        {Status::Paused, Status::Failed, true},
        {Status::Paused, Status::Canceled, true},
        {Status::Completed, Status::Running, false},
        {Status::Failed, Status::Running, false},
        {Status::Canceled, Status::Running, false},
        {static_cast<Status>(99), Status::Running, false}
    };

    for (const TransitionCase &testCase : cases) {
        const QString label = QStringLiteral("%1 -> %2")
            .arg(static_cast<int>(testCase.from))
            .arg(static_cast<int>(testCase.to));
        QVERIFY2(TaskManager::isValidTransition(testCase.from, testCase.to)
                     == testCase.expected,
                 qPrintable(label));
    }
}

void TaskManagerStateTest::startTaskKeepsLocalStateWithoutGateway()
{
    TaskManager manager(nullptr);
    const QString taskId = manager.addDownloadTask(
        QStringLiteral("archive.zip"),
        QStringLiteral("C:/non-existent-download-path"),
        QStringLiteral("node-1"),
        128);

    QVERIFY(!taskId.isEmpty());
    QVERIFY(manager.canStartTask(taskId));
    QVERIFY(manager.startTask(taskId));

    TaskManager::DownloadTask task;
    QVERIFY(manager.getDownloadTask(taskId, task));
    QCOMPARE(task.status, static_cast<int>(TaskManager::TaskStatus::Running));
    QVERIFY(!manager.canStartTask(taskId));
}

void TaskManagerStateTest::terminalAndUnknownTasksRejectUpdates()
{
    TaskManager manager(nullptr);
    const QString taskId = manager.addDownloadTask(
        QStringLiteral("archive.zip"),
        QStringLiteral("C:/non-existent-download-path"),
        QStringLiteral("node-1"),
        128);

    QVERIFY(manager.startTask(taskId));
    QVERIFY(manager.updateTaskStatusAtomically(
        taskId, static_cast<int>(TaskManager::TaskStatus::Completed)));

    TaskManager::DownloadTask terminalTask;
    QVERIFY(manager.getDownloadTask(taskId, terminalTask));
    QCOMPARE(terminalTask.status, static_cast<int>(TaskManager::TaskStatus::Completed));

    QVERIFY(!manager.updateTaskStatusAtomically(
        taskId, static_cast<int>(TaskManager::TaskStatus::Running)));
    QVERIFY(!manager.updateTaskStatusAtomically(
        QStringLiteral("missing-task"), static_cast<int>(TaskManager::TaskStatus::Running)));

    TaskManager::DownloadTask unchangedTask;
    QVERIFY(manager.getDownloadTask(taskId, unchangedTask));
    QCOMPARE(unchangedTask.status, static_cast<int>(TaskManager::TaskStatus::Completed));
}

void TaskManagerStateTest::gatewayReceivesTransferRequests()
{
    FakeTaskTransferGateway gateway;
    TaskManager manager(&gateway);
    const QString taskId = manager.addDownloadTask(
        QStringLiteral("archive.zip"),
        QStringLiteral("C:/non-existent-download-path"),
        QStringLiteral("node-7"),
        128);

    QVERIFY(manager.startTask(taskId));
    QCOMPARE(gateway.startedRequests.size(), 1);

    const TaskTransferGateway::TransferRequest request = gateway.startedRequests.constFirst();
    QCOMPARE(request.type, TaskTransferGateway::TransferType::Download);
    QCOMPARE(request.fileName, QStringLiteral("archive.zip"));
    QCOMPARE(request.savePath, QStringLiteral("C:/non-existent-download-path"));
    QCOMPARE(request.nodeId, QStringLiteral("node-7"));
    QCOMPARE(request.taskId, taskId);
    QCOMPARE(request.threadCount, 1);
    QCOMPARE(request.startOffset, qint64(0));

    manager.pauseTask(taskId);
    QCOMPARE(gateway.controlCalls.size(), 1);
    QCOMPARE(gateway.controlCalls.constFirst().taskId, taskId);
    QCOMPARE(gateway.controlCalls.constFirst().type, TaskTransferGateway::TransferType::Download);
    QCOMPARE(gateway.controlCalls.constFirst().action,
             TaskTransferGateway::TransferControlAction::Pause);

    manager.cancelTask(taskId);
    QCOMPARE(gateway.controlCalls.size(), 2);
    QCOMPARE(gateway.controlCalls.constLast().action,
             TaskTransferGateway::TransferControlAction::Cancel);
}

QTEST_MAIN(TaskManagerStateTest)
#include "task_manager_state_test.moc"
