/*
 * 主窗口组合根实现：装配模块、路由页面和汇总全局 UI 反馈。
 * 跨模块连接属于组合职责；页面校验、统计和弹窗仍有存量逻辑，不能视为纯装配器。
 */
#include "FileItem.h"
#include "HomeDownloadSelectionPolicy.h"
#include "HomeFileRowPresentation.h"
#include "HomeTaskErrorPolicy.h"
#include "HomeTaskStatusPolicy.h"
#include "HomeWidge.h"
#include "Database.h"
#include "NetWork.h"
#include "DirectoryGateway.h"
#include "NodeGateway.h"
#include "TaskTransferGateway.h"
#include "TaskNodeNameGateway.h"
#include "TaskCreationGateway.h"
#include "TaskManager.h"
#include "FileBrowser.h"
#include "TaskItem.h"
#include "NodeDialog.h"
#include "DirectoryNavigator.h"
#include "DirectoryModule.h"
#include "DirectoryPageController.h"
#include "NodeModule.h"
#include "NodePageController.h"
#include "UploadModule.h"
#include "UploadController.h"
#include "DownloadModule.h"
#include "DownloadController.h"
#include "TaskListController.h"
#include "TaskModule.h"
#include "ui_HomeWidge.h"
#include <QSize>
#include <QColor>
#include <QPalette>

#include <QFileDialog>
#include <QLineEdit>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <QTabBar>
#include <algorithm>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QAbstractItemModel>

HomeWidge::HomeWidge(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HomeWidge)
    , lastSavePath(QDir::homePath())
{
    ui->setupUi(this);

    const auto makeUploadPageOpaque = [](QWidget *widget) {
        if (!widget) {
            return;
        }
        widget->setAttribute(Qt::WA_StyledBackground, true);
        widget->setAutoFillBackground(true);
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Window, QColor(QStringLiteral("#F7F9FC")));
        widget->setPalette(palette);
    };
    makeUploadPageOpaque(ui->UploadWidget);
    makeUploadPageOpaque(ui->UploadStackWidget);
    makeUploadPageOpaque(ui->UploadStep1);
    makeUploadPageOpaque(ui->UploadStep2);

    // 主页完成模块装配和页面级桥接；具体网络/任务算法继续由对应模块负责。
    nw = new NetWork(this);
    db = new Database(this);
    m_nodeGateway = new NodeGateway(nw, this);
    m_directoryGateway = new DirectoryGateway(nw, this);
    m_taskTransferGateway = new TaskTransferGateway(nw, this);
    m_taskNodeNameGateway = new TaskNodeNameGateway(m_nodeGateway, this);
    m_taskManager = new TaskManager(m_taskTransferGateway, this);
    m_taskCreationGateway = new TaskCreationGateway(m_taskManager, this);
    m_fileBrowser = new FileBrowser(m_directoryGateway, m_nodeGateway, this);
    m_directoryNavigator = new DirectoryNavigator(this);
    m_directoryModule = new DirectoryModule(this);
    m_directoryPageController = nullptr;
    m_nodeModule = new NodeModule(db, m_nodeGateway, this);
    m_nodePageController = nullptr;
    m_uploadModule = nullptr;
    m_uploadController = nullptr;
    m_downloadModule = nullptr;
    m_downloadController = nullptr;
    m_taskListController = nullptr;
    m_taskModule = nullptr;

    m_directoryPageController = new DirectoryPageController(ui->NodeTabWidget,
                                                             ui->DownloadNodeComboBox,
                                                             m_fileBrowser,
                                                             m_nodeGateway,
                                                             m_directoryGateway,
                                                            m_directoryModule,
                                                            this,
                                                            this);
    m_nodePageController = new NodePageController(ui->NodeTabWidget,
                                                  getDefaultNodePage(),
                                                  m_fileBrowser,
                                                  m_directoryPageController,
                                                  this);
    m_uploadModule = new UploadModule(ui->CheckBackupNode,
                                      m_nodeGateway,
                                      m_taskCreationGateway,
                                      this,
                                      this);
    m_nodeModule->setUiComponents(ui->listWidget,
                                  ui->DownloadNodeComboBox,
                                  ui->CheckBackupNode);
    m_uploadController = new UploadController(ui->FilesTree,
                                              ui->CheckBackupNode,
                                              ui->CheckAll,
                                              ui->UploadStackWidget,
                                              ui->uploadHeaderName,
                                              ui->uploadHeaderType,
                                              ui->uploadHeaderSize,
                                              ui->uploadHeaderPath,
                                              m_uploadModule,
                                              this,
                                              this);
    m_downloadModule = new DownloadModule(ui->DownloadNodeComboBox,
                                          this,
                                          m_nodeGateway,
                                          m_taskCreationGateway,
                                          &lastSavePath,
                                          this);
    m_downloadController = new DownloadController(ui->DownloadNodeComboBox,
                                                  this,
                                                  m_downloadModule,
                                                  this);
    m_taskListController = new TaskListController(ui->DownloadingList,
                                                  ui->FinishedList,
                                                  ui->SelectAllCheckBox,
                                                  ui->SelectAllFinishedCheckBox,
                                                  this);
    m_taskModule = new TaskModule(m_taskManager,
                                  m_taskListController,
                                  m_taskNodeNameGateway,
                                  ui->DownloadingList,
                                  ui->FinishedList,
                                  this);

    bindTaskSignals();
    bindDirectorySignals();
    bindNodeSignals();
    bindTransferSignals();

    GuiInit();
    EventInit();
    DatabaseInit();
    LoadNodesFromDatabase();
    on_change_stackedWidget(0);
}

void HomeWidge::bindTaskSignals()
{
    connect(m_taskManager, &TaskManager::taskProgressChanged,
            this, &HomeWidge::handleTaskProgressChanged);
    connect(m_taskManager, &TaskManager::taskStatusChanged,
            this, &HomeWidge::handleTaskStatusChanged);
    connect(nw, &NetWork::taskError, this, &HomeWidge::showTaskErrorMessage);
    connect(m_taskManager, &TaskManager::taskError, this, &HomeWidge::showTaskErrorMessage);

    connect(m_taskListController, &TaskListController::taskDeleteRequested, this, [this](const QString &taskId) {
        if (m_taskModule) {
            m_taskModule->deleteTaskById(taskId);
        }
    });
    connect(m_taskListController, &TaskListController::taskControlRequested, this, [this](const QString &taskId) {
        if (!m_taskModule) {
            return;
        }
        const TaskModule::ActionResult result = m_taskModule->toggleTaskById(taskId);
        if (!result.message.isEmpty()) {
            showTaskControlMessage(QStringLiteral("提示"), result.message, result.warning);
        }
    });
}

void HomeWidge::bindDirectorySignals()
{
    connect(nw, &NetWork::fileListUpdated, this, &HomeWidge::handleFileListUpdated);
    connect(m_directoryModule, &DirectoryModule::navigateToPath,
            this, &HomeWidge::handleBreadcrumbNavigate);

    connect(m_fileBrowser, &FileBrowser::fileListLoaded, this, [this](const QString &, const QString &) {
        bindDownloadTreeSelectionSync(currentDownloadTree());
        syncDownloadSelectAllState();
    });
    connect(m_fileBrowser, &FileBrowser::loadError, this, [this](const QString &, const QString &error) {
        QMessageBox::warning(this, "加载错误", error);
    });
    connect(ui->NodeTabWidget, &QTabWidget::currentChanged, this, [this](int) {
        bindDownloadTreeSelectionSync(currentDownloadTree());
        syncDownloadSelectAllState();
    });
}

void HomeWidge::bindNodeSignals()
{
    connect(m_nodeModule, &NodeModule::checkedNodeItemsChanged, this, [this](int checkedCount, int totalCount) {
        checkNodeItems = m_nodeModule->checkedNodeItems();
        ui->CheckAllNode->setChecked(totalCount > 0 && checkedCount == totalCount);
    });
}

void HomeWidge::bindTransferSignals()
{
    connect(m_uploadController, &UploadController::uploadTaskCreated, this, [this](const QString &taskId, const QString &filePath) {
        if (!m_taskModule) {
            return;
        }

        m_taskModule->appendCreatedTaskAndStart(taskId,
                                                QFileInfo(filePath).fileName());
        on_change_stackedWidget(2);
    });
    connect(m_uploadController, &UploadController::switchToTaskPageRequested, this, [this]() {
        on_change_stackedWidget(2);
    });
    connect(m_downloadController, &DownloadController::downloadTaskCreated, this,
            [this](const QString &taskId, const QString &fileName) {
                if (m_taskModule) {
                    m_taskModule->appendCreatedTaskAndStart(taskId, fileName);
                    on_change_stackedWidget(2);
                }
            });
    connect(m_downloadController, &DownloadController::switchToTaskPageRequested, this, [this]() {
        on_change_stackedWidget(2);
    });
    connect(m_uploadController, &UploadController::uploadSelectionChanged, this, [this](int checkedCount, int totalCount) {
        Q_UNUSED(checkedCount);
        Q_UNUSED(totalCount);
        checkFileItems.clear();
        for (int i = 0; i < ui->FilesTree->count(); ++i) {
            QListWidgetItem *item = ui->FilesTree->item(i);
            FileItem *fileItem = item ? qobject_cast<FileItem*>(ui->FilesTree->itemWidget(item)) : nullptr;
            if (fileItem && fileItem->getCheckStatus() == Qt::Checked) {
                checkFileItems[i] = fileItem;
            }
        }
    });
}

HomeWidge::~HomeWidge()
{
    delete ui;
    delete db;
}

void HomeWidge::DatabaseInit()
{
    QString dbPath = QCoreApplication::applicationDirPath() + "/backup.db";
    if (!db->Initialize(dbPath)) {
        QMessageBox::warning(this, "警告", "数据库初始化失败，节点信息将无法保存");
    }
}

void HomeWidge::LoadNodesFromDatabase()
{
    if (m_nodeModule) {
        m_nodeModule->loadNodesFromDatabase();
    }
    if (m_nodePageController) {
        m_nodePageController->resetDefaultNodePageDisplay();
        m_nodePageController->configureDefaultFolderTree();
    }
}

void HomeWidge::GuiInit()
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAutoFillBackground(true);
    QPalette windowPalette = palette();
    windowPalette.setColor(QPalette::Window, QColor(QStringLiteral("#F4F7FB")));
    setPalette(windowPalette);
    setAcceptDrops(true);

    const QList<QPushButton*> navigationButtons = {
        ui->Upload, ui->Download, ui->Tasklist, ui->Nodelist
    };
    for (QPushButton *button : navigationButtons) {
        button->setProperty("nav", true);
        button->setCheckable(true);
        button->setAutoExclusive(true);
    }

    const QList<QPushButton*> primaryButtons = {
        ui->EnterUpload, ui->DownloadFolder, ui->StartDownload,
        ui->NewFolder, ui->NewNode
    };
    for (QPushButton *button : primaryButtons) {
        button->setProperty("buttonRole", "primary");
    }

    const QList<QPushButton*> dangerButtons = {
        ui->RemoveFiles, ui->RemoveAll, ui->CancelDownload,
        ui->DeleteFinishedButton, ui->RemoveNode
    };
    for (QPushButton *button : dangerButtons) {
        button->setProperty("buttonRole", "danger");
    }

    const QList<QPushButton*> ghostButtons = {
        ui->Minibutton, ui->Closebutton, ui->BackStepButton,
        ui->CancelUpload, ui->PauseDownload
    };
    for (QPushButton *button : ghostButtons) {
        button->setProperty("buttonRole", "ghost");
    }

    ui->ChangeNode->setProperty("buttonRole", "secondary");

    if (ui->NodeTabWidget && ui->NodeTabWidget->tabBar()) {
        ui->NodeTabWidget->tabBar()->setDrawBase(false);
    }

    if (ui->TaskStackedWidget && ui->TaskStackedWidget->tabBar()) {
        ui->TaskStackedWidget->tabBar()->setDrawBase(false);
    }
}

void HomeWidge::EventInit()
{
    connect(ui->Upload,&QPushButton::clicked,this,[=]{on_change_stackedWidget(0);});
    connect(ui->Download,&QPushButton::clicked,this,[=]{on_change_stackedWidget(1);});
    connect(ui->Tasklist,&QPushButton::clicked,this,[=]{on_change_stackedWidget(2);});
    connect(ui->Nodelist,&QPushButton::clicked,this,[=]{on_change_stackedWidget(3);});

    connect(ui->StartDownload, &QPushButton::clicked, this, &HomeWidge::startDownload);
    connect(ui->PauseDownload, &QPushButton::clicked, this, &HomeWidge::pauseDownload);
    connect(ui->CancelDownload, &QPushButton::clicked, this, &HomeWidge::cancelDownload);
    connect(ui->SelectAllCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        onSelectAllCheckBoxChanged(static_cast<int>(state));
    });

    connect(ui->SelectAllFinishedCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        onSelectAllFinishedCheckBoxChanged(static_cast<int>(state));
    });
    connect(ui->DeleteFinishedButton, &QPushButton::clicked, this, &HomeWidge::deleteSelectedFinishedTasks);
    connect(ui->BackStepButton, &QPushButton::clicked, this, [=]() {
        ui->UploadStackWidget->setCurrentIndex(0);
    });
    if (m_directoryPageController) {
        m_directoryPageController->bindDirectoryInteractions(getDefaultNodePage());
    }
}

void HomeWidge::bindDownloadTreeSelectionSync(QTreeWidget *treeWidget)
{
    if (!treeWidget) {
        return;
    }
    if (treeWidget->property("downloadSelectSyncBound").toBool()) {
        return;
    }
    treeWidget->setProperty("downloadSelectSyncBound", true);

    // 默认下载页和动态目录页签共用这条全选同步链，避免每个页面重复绑定。
    auto syncIfCurrentTabTree = [this, treeWidget]() {
        QWidget *currentTab = ui->NodeTabWidget ? ui->NodeTabWidget->currentWidget() : nullptr;
        if (currentTab && currentTab->isAncestorOf(treeWidget)) {
            syncDownloadSelectAllState();
        }
    };

    connect(treeWidget, &QTreeWidget::itemChanged, this,
            [syncIfCurrentTabTree](QTreeWidgetItem *, int) {
                syncIfCurrentTabTree();
            });
    connect(treeWidget, &QTreeWidget::itemClicked, this,
            [syncIfCurrentTabTree](QTreeWidgetItem *, int) {
                syncIfCurrentTabTree();
            });
    connect(treeWidget, &QTreeWidget::itemPressed, this,
            [syncIfCurrentTabTree](QTreeWidgetItem *, int) {
                QTimer::singleShot(0, syncIfCurrentTabTree);
            });
    if (treeWidget->model()) {
        connect(treeWidget->model(), &QAbstractItemModel::dataChanged, this,
                [syncIfCurrentTabTree](const QModelIndex &, const QModelIndex &, const QList<int> &) {
                    QTimer::singleShot(0, syncIfCurrentTabTree);
                });
    }
}
// 同步下载节点全选状态
void HomeWidge::syncDownloadSelectAllState() const
{
    if (!ui->CheckAllFolder) {
        return;
    }

    QWidget *currentTab = ui->NodeTabWidget ? ui->NodeTabWidget->currentWidget() : nullptr;
    QTreeWidget *treeWidget = currentTab ? currentTab->findChild<QTreeWidget*>("folderTree") : nullptr;
    QList<Qt::CheckState> itemStates;
    if (treeWidget) {
        itemStates.reserve(treeWidget->topLevelItemCount());
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            const QTreeWidgetItem *item = treeWidget->topLevelItem(i);
            itemStates.append(item ? item->checkState(0) : Qt::Unchecked);
        }
    }
    const bool allChecked = HomeDownloadSelectionPolicy::allItemsChecked(itemStates);

    ui->CheckAllFolder->blockSignals(true);
    ui->CheckAllFolder->setChecked(allChecked);
    ui->CheckAllFolder->blockSignals(false);
}
