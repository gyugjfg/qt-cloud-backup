#ifndef HOMEWIDGE_H
#define HOMEWIDGE_H

#include "NetworkTypes.h"
#include "TaskFeedbackSummary.h"

#include <QWidget>
#include <QMap>
#include <QList>
#include <QSet>
#include <QPointF>

QT_BEGIN_NAMESPACE
namespace Ui {
class HomeWidge;
}
QT_END_NAMESPACE

class NetWork;
class Database;
class DirectoryGateway;
class NodeGateway;
class TaskManager;
class TaskTransferGateway;
class TaskNodeNameGateway;
class TaskCreationGateway;
class FileBrowser;
class TaskListController;
class TaskModule;
class DirectoryNavigator;
class DirectoryModule;
class DirectoryPageController;
class NodeModule;
class NodePageController;
class UploadModule;
class UploadController;
class DownloadModule;
class DownloadController;
class QTreeWidget;
class QMouseEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class HomeWidge : public QWidget
{
    Q_OBJECT

public:
    HomeWidge(QWidget *parent = nullptr);
    ~HomeWidge();

protected:
    void GuiInit();
    void EventInit();
    void DatabaseInit();
    void LoadNodesFromDatabase();

    void TestValue();

    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void on_Closebutton_clicked();
    void on_Minibutton_clicked();
    void on_CheckFiles_clicked();
    void on_DownloadNodeComboBox_currentIndexChanged(int index);

    void on_EnterUpload_clicked();

    void on_CancelUpload_clicked();

    void on_CheckAll_checkStateChanged(const Qt::CheckState &arg1);

    void on_RemoveAll_clicked();

    void on_RemoveFiles_clicked();

    void on_AppendFiles_clicked();

    void on_NewFolder_clicked();

    void on_NewNode_clicked();
    void on_RemoveNode_clicked();
    void on_ChangeNode_clicked();
    void on_CheckAllNode_clicked();
    void on_CheckAllFolder_clicked();

    void on_DownloadFolder_clicked();

    void startDownload();
    void pauseDownload();
    void cancelDownload();
    void onSelectAllCheckBoxChanged(int state);
    void onSelectAllFinishedCheckBoxChanged(int state);
    void deleteSelectedFinishedTasks();

protected slots:
    void changeCheckedItem(bool status,QWidget *item);
    void handleTaskProgressChanged(const QString &taskId, int progress, qint64 transferredSize, qint64 totalSize, double speed);
    void handleTaskStatusChanged(const QString &taskId, int status);
    void handleFileListUpdated(const QString &nodeId, const QList<NetworkFileInfo> &fileList);
    void handleBreadcrumbNavigate(const QString &nodeId, const QString &path);
    void showTaskErrorMessage(const QString &taskId, const QString &errorMessage);

private:
    // 主页只保留默认页装配入口，默认节点页细节继续交给节点/目录控制层。
    QWidget *getDefaultNodePage() const;
    void resetDefaultNodePage();
    void on_change_stackedWidget(int index);
    void updateDefaultNodeTabTitle(const QString &nodeName, const QString &path);
    QTreeWidget *currentDownloadTree() const;
    void showTaskControlMessage(const QString &title, const QString &message, bool warning = false) const;
    // 任务汇总仍留在主页，避免任务层反向依赖弹窗和页面文案。
    void recordTaskCompletion(const QString &fileName, const QString &nodeName, bool isDownload, int status);
    // 下载默认页和动态目录页签共用同一套全选联动，先由主页统一收口。
    void bindDownloadTreeSelectionSync(QTreeWidget *treeWidget);
    void syncDownloadSelectAllState() const;

    // 组合根只负责装配跨模块信号；按业务边界分组，避免构造函数隐藏页面协同关系。
    void bindTaskSignals();
    void bindDirectorySignals();
    void bindNodeSignals();
    void bindTransferSignals();

    // 汇总弹窗属于主页级反馈，不下沉到任务主线。
    void showTaskSummary();

private:
    Ui::HomeWidge *ui;

    bool    isDrag;
    QPointF dVal;

    QMap<int,QWidget*> checkFileItems;
    QMap<int,QWidget*> checkNodeItems;

    // 主页负责组合根和跨模块协同；历史页面校验、统计和弹窗仍有少量存量逻辑。
    TaskManager *m_taskManager;
    TaskTransferGateway *m_taskTransferGateway;
    NodeGateway *m_nodeGateway;
    DirectoryGateway *m_directoryGateway;
    TaskNodeNameGateway *m_taskNodeNameGateway;
    TaskCreationGateway *m_taskCreationGateway;
    FileBrowser *m_fileBrowser;
    DirectoryNavigator *m_directoryNavigator;
    DirectoryModule *m_directoryModule;
    DirectoryPageController *m_directoryPageController;
    NodeModule *m_nodeModule;
    NodePageController *m_nodePageController;
    UploadModule *m_uploadModule;
    UploadController *m_uploadController;
    DownloadModule *m_downloadModule;
    DownloadController *m_downloadController;
    TaskListController *m_taskListController;
    TaskModule *m_taskModule;

    // 下载保存目录仍由主页记住最近一次选择，避免模块之间重复维护这一份 UI 状态。
    QString lastSavePath;

    NetWork *nw;
    Database *db;

    // 主页只持有纯反馈汇总，不再维护上传/下载两套重复计数结构。
    TaskFeedbackSummary m_taskFeedbackSummary;
    // 同一批底层错误可能会从多条链路回流，主页统一做一次弹窗去重。
    QSet<QString> m_activeTaskErrorKeys;
};

#endif // HOMEWIDGE_H
