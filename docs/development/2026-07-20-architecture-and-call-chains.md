# 模块总览：架构与业务调用链

## 基本信息

* **模块状态**：结构整理和调用链记录已完成；当前客户端 Release 构建、真实 Linux `server.cpp` 上传/断点下载联调已验证，Qt UI 全链路仍按证据边界管理。

* **用户价值**：让维护者或面试官可以从一次用户操作追到具体对象、状态、线程和失败回流，而不是只看到目录箭头。

* **范围**：客户端启动、节点、远程目录、上传/下载任务、任务控制、进度回流、SQLite 节点持久化和 Linux socket 服务端协议。

* **不包含**：不改变 `.ui` 布局、控件 `objectName`、用户文案、网络协议字段、数据库 schema、线程策略或任务状态语义；不把未联调功能写成已交付产品。

## 一、架构分层

```text
src/app/main.cpp
    |
    v
src/ui/shell/HomeWidge   <- 组合根、页面路由、全局 UI 反馈
    |
    +--> features/nodes       -> Database + NodeGateway
    +--> features/directory   -> DirectoryGateway + NodeGateway
    +--> features/transfers  -> TaskCreationGateway / TaskTransferGateway
    |
    v
src/core/network/NetWork    <- 兼容门面，只在 Gateway 和组合根接触
    +--> NodeService
    +--> DirectoryService
    +--> TransferService
          +--> TransferProtocolClient
          +--> TransferControlState
    |
    +--> Qt Network / SQLite / socket server
```

### 层职责与禁止事项

| 层                    | 主要对象                                                                                                                        | 负责什么                                     | 不应负责什么                |
| -------------------- | --------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- | --------------------- |
| `app`                | `main.cpp`                                                                                                                  | 创建 `QApplication`、加载主题、创建主窗口             | 不拼接 SQL、不创建 socket 任务 |
| `ui/shell`           | `HomeWidge`、`TitleBar`、`HomeFileRowPresentation`                                                                            | 组合对象、路由页面、消费结果并更新 QWidget；纯文件行展示映射       | 不实现传输算法和数据库细节         |
| `features/nodes`     | `NodeModule`、`NodePageController`                                                                                           | 节点输入、列表和选择器同步                            | 不直接包含 `NetWork.h`     |
| `features/directory` | `FileBrowser`、`DirectoryPageController`                                                                                     | 路径导航、目录树和文件选择                            | 不在线程池里创建 QWidget      |
| `features/transfers` | `UploadModule`、`DownloadModule`、`TaskModule`、`TaskManager`                                                                  | 创建任务、状态规则、任务列表语义和控制意图                    | 不让页面直接操作 socket       |
| `core/network`       | `NodeGateway`、`DirectoryGateway`、`TaskTransferGateway`、`NetWork`、三个 Service、`TransferProtocolClient`、`TransferControlState` | 对外提供能力端口，执行网络/线程/socket 工作；协议原语和传输控制状态分离 | 不放页面文案和 QWidget       |
| `core/persistence`   | `Database`                                                                                                                  | SQLite 连接、schema 和节点 CRUD                | 不维护 UI 状态机            |
| `server.cpp`         | Linux 独立进程                                                                                                                  | 解析命令、规范化路径、读写文件和返回协议结果                   | 不属于客户端 qmake target   |

## 二、对象关系、所有权和线程

### 组合根创建顺序

```text
HomeWidge
  -> NetWork
  -> Database
  -> NodeGateway / DirectoryGateway
  -> TaskTransferGateway / TaskNodeNameGateway
  -> TaskManager
  -> TaskCreationGateway
  -> FileBrowser / DirectoryNavigator / Feature Modules / Controllers
```

* `HomeWidge` 通过 QObject parent 拥有上述动态对象；销毁顺序与创建顺序相反。

* `NodeGateway`、`DirectoryGateway`、`TaskTransferGateway`、`TaskNodeNameGateway` 和 `TaskCreationGateway` 都只借用下游对象，不拥有 `NetWork` 或 `TaskManager`。

* Controller/Module 借用由 `.ui` 创建的 QWidget；它们不负责删除 Designer 控件。

* `TaskManager` 的任务快照在进程内 `QMap` 中保存，`QMutex` 保护并发读写；当前任务运行时不是完整 SQLite 恢复闭环。

* `NodeService`、`DirectoryService`、`TransferService` 使用 `QThreadPool` 或 socket 工作；网络结果通过 Qt 信号和 queued callback 回到对象线程，UI 只在主线程更新。`TransferControlState` 不拥有线程或 socket，只受 `TransferService` 控制锁保护。

* `TaskCreationGateway` 和 `TaskTransferGateway` 不创建线程；它们只收窄入口和转发值对象/信号。

## 三、核心数据身份和状态

### 贯穿调用链的身份

| 身份            | 首次产生/读取                                      | 后续用途                   |
| ------------- | -------------------------------------------- | ---------------------- |
| `nodeId`      | 节点创建或 SQLite 加载                              | 节点查询、目录请求、任务目标、服务端连接选择 |
| `taskId`      | `TaskManager::addUploadTask/addDownloadTask` | 状态、进度、控制、错误和完成回流的唯一键   |
| `filePath`    | 上传控件或远程目录项                                   | 上传本地源，或下载远程路径          |
| `savePath`    | 下载保存目录与文件名                                   | 下载本地目标文件               |
| `startOffset` | `TaskManager::sync*TransferredBytesLocked`   | 暂停/恢复或已有部分文件时构造续传请求    |

### 任务状态边界

| 状态          | 含义         | 允许动作           | 事实保存位置                            |
| ----------- | ---------- | -------------- | --------------------------------- |
| `Waiting`   | 已登记、尚未下发传输 | 开始、取消          | `TaskManager` 内存                  |
| `Running`   | 传输执行中      | 暂停、取消          | `TaskManager` + `TransferService` |
| `Paused`    | 暂停或保留偏移    | 恢复、取消          | 内存快照 + 本地文件偏移                     |
| `Completed` | 传输完成       | 展示、删除          | 内存快照                              |
| `Failed`    | 传输或协议失败    | 由现有 UI 决定是否再处理 | 内存快照                              |
| `Canceled`  | 用户取消       | 展示、删除          | 内存快照                              |

`TaskManager::TaskStatus` 是 `NetworkTransferStatus` 的业务别名；`TaskManager::isValidTransition` 和 `updateTaskStatusAtomically` 是状态规则入口。Gateway 不自行改变任务状态；`TransferService` 只报告传输结果，状态归并仍由任务主线负责。

## 四、业务调用链

### 1. 启动、数据库初始化和节点加载

```text
main()
  -> QApplication / 主题
  -> HomeWidge::HomeWidge()
      -> NetWork / Database / Gateways / Modules
      -> GuiInit / EventInit
      -> DatabaseInit
          -> Database::Initialize
              -> Database::CreateTables
      -> LoadNodesFromDatabase
          -> NodeModule::loadNodesFromDatabase
              -> Database::GetAllNodes
              -> NodeGateway::addNode
              -> NodeModule::appendNodeListItem / addNodeToSelectors
  -> on_change_stackedWidget(0)
  -> QApplication::exec()
```

**输入和结果**：输入是应用启动和数据库路径；结果是节点列表、上传/下载节点选择器和默认页初始化。

**失败保持**：数据库初始化失败由 `Database` 返回失败；当前 UI 启动错误提示和节点空状态必须以代码实际行为为准，不能凭文档假设为后台重试。

**必须走读**：`src/app/main.cpp::main`、`HomeWidge::HomeWidge`、`HomeWidge::DatabaseInit`、`HomeWidge::LoadNodesFromDatabase`、`NodeModule::loadNodesFromDatabase`、`Database::Initialize`。

### 2. 新增/修改/删除节点

```text
用户点击节点按钮
  -> HomeWidge::on_NewNode_clicked / on_ChangeNode_clicked / on_RemoveNode_clicked
  -> NodeModule::createNode / updateSelectedNode / removeSelectedNodes
  -> NodeDialog 收集输入
  -> NodeGateway::addNode/updateNode/removeNode
  -> Database::AddNode/UpdateNode/DeleteNode
  -> NodeModule 更新 QListWidget 与两个 QComboBox
```

* `NodeGateway` 只暴露节点能力，不把目录或传输接口带进节点 Feature。

* 数据库写入和内存网络节点写入是两个当前步骤；没有统一事务，不能把它描述成跨存储原子提交。

* 删除节点还会触发网络层停止该节点的目录同步；目录树、任务快照和已存在本地文件的后续影响需要单独验证。

### 3. 远程目录读取、导航和自动刷新

```text
用户打开目录页/切换路径
  -> DirectoryPageController::navigateDirectoryTab
  -> FileBrowser::loadFileList(nodeId, path, tree)
  -> DirectoryGateway::fileInfoList
  -> NetWork::GetFileInfoList
  -> DirectoryService::getFileInfoList
  -> NodeService::getConnection + socket 请求
  -> FileBrowser 在 UI 线程填充 QTreeWidget
```

自动刷新链路：

```text
autoSyncCheckBox
  -> DirectoryGateway::startFileListSync/stopFileListSync
  -> NetWork
  -> DirectoryService 定时任务
  -> DirectoryGateway::fileListUpdated
  -> FileBrowser::handleFileListUpdated
```

* 线程池只返回 `NetworkFileInfo` 值对象；禁止在工作线程触碰 QWidget。

* `FileBrowser` 用请求序号和树指针记录最后一次加载，旧结果不能覆盖新路径。

* `DirectoryPageController::openDirectorySelectionDialog` 的线程池和 queued 回调通过 `QPointer` 守卫控制器、Gateway、FileBrowser 和对话框；请求序号由共享对象延长到异步回调完成。

* 空列表需要区分“节点离线”和“节点在线但没有文件”；当前界面文案由 `FileBrowser::fillTreeWidget` 决定。

**必须走读**：`DirectoryPageController::navigateDirectoryTab`、`openDirectorySelectionDialog`、`FileBrowser::loadFileList`、`FileBrowser::fillTreeWidget`、`DirectoryService::getFileInfoList`、`DirectoryService::startFileListSync`。

### 4. 上传任务创建

```text
用户选择本地文件并点击下一步/上传
  -> UploadController::startSelectedUploads
  -> UploadModule::createUploadTasks
      -> selectedUploadNodeId / NodeGateway::nodeInfo
      -> NodeGateway::checkNodeStatus
      -> TaskCreationGateway::createUploadTask
      -> TaskManager::addUploadTask
      -> emit UploadModule::uploadTaskCreated(taskId, filePath)
      -> UploadController 转发 uploadTaskCreated
      -> HomeWidge::bindTransferSignals lambda
      -> TaskModule::appendCreatedTaskAndStart
      -> TaskManager::startTask
```

* `UploadModule` 负责前置校验和批量遍历；`TaskCreationGateway` 只负责创建，不负责启动或控制。

* `taskId` 在任务登记时产生，后续页面跳转和任务列表都应使用同一个 ID。

* 节点为空、节点不存在、节点离线、文件路径为空或创建端口不可用时，不能发出伪造的有效任务 ID。

### 5. 下载任务创建

```text
用户在 QTreeWidget 勾选远程文件
  -> DownloadController::prepareDownloads
  -> DownloadModule::createDownloadTasks
      -> ensureDownloadNodeReady
      -> checkedDownloadItems
      -> 过滤目录项
      -> selectDownloadSaveDirectory
      -> confirmDownloadOverwrite
      -> TaskCreationGateway::createDownloadTask
      -> TaskManager::addDownloadTask
      -> emit downloadTaskCreated(taskId, fileName)
      -> HomeWidge -> TaskModule::appendCreatedTaskAndStart
```

* 远程路径、显示文件名、本地保存目录和节点 ID 是四个不同语义，不应合并成一个字符串。

* 覆盖选择为 skip/cancel 时不创建任务；成功创建的数量和用户取消/跳过提示由现有模块保持。

* 目录项不属于当前下载任务能力，必须在入队前拒绝。

### 6. 任务启动、暂停、恢复和取消

启动：

```text
TaskModule::appendCreatedTaskAndStart
  -> TaskManager::startTask
  -> startUploadTaskLocked / startDownloadTaskLocked
  -> sync*TransferredBytesLocked
  -> build*TransferRequest
  -> TaskTransferGateway::startTransfer
  -> NetWork::StartTransferAsync
  -> TransferService::startTransferAsync
  -> processTaskQueue
```

控制：

```text
TaskListController::taskControlRequested
  -> HomeWidge::bindTaskSignals
  -> TaskModule::toggleTaskById
  -> TaskManager::pauseTask/cancelTask/resumeTask
  -> TaskTransferGateway::controlTransfer
  -> NetWork::ControlTransfer
  -> TransferService::controlTransfer
```

* `TaskManager` 先检查任务是否存在和状态是否合法，再向下游发送控制请求。

* 暂停保留可恢复偏移；取消是终止语义；失败来自传输或协议错误；三者不能在面试中混为“任务结束”。

* `TaskTransferGateway` 不拥有 `NetWork`，也不在控制调用中直接改 UI 或弹窗。

### 7. 进度、状态、完成和错误回流

```text
TransferService
  -> taskProgressChanged/taskStatusChanged/taskError
  -> NetWork queued forwarding
  -> TaskTransferGateway forwarding
  -> TaskManager updateTaskProgress/updateTaskStatus/taskError
  -> TaskModule / TaskItem 更新列表
  -> HomeWidge::handleTaskProgressChanged/handleTaskStatusChanged
  -> 任务统计、完成列表和错误弹窗
```

* 进度信号携带 `taskId`，不能只依赖当前选中的列表项。

* 主页统计是展示投影，不是任务状态事实来源；事实仍在 `TaskManager` 快照和传输回流。

* 错误可能由多条路径回到主页，主页现有去重集合负责避免重复弹窗。

**必须走读**：`TaskManager::updateTaskProgress`、`updateTaskStatusAtomically`、`TaskModule::handleTaskCompletion`、`HomeWidge::handleTaskProgressChanged`、`HomeWidge::handleTaskStatusChanged`、`TransferService::send*Signal`。

## 五、持久化与协议边界

### SQLite

`Database` 当前真实主用闭环是节点表初始化和 CRUD；`files`、`tasks` 表及部分接口存在，但没有接入完整任务重启恢复。因而“有 tasks 表”不能写成“支持任务恢复”。

### 客户端网络

`NetWork` 仍是兼容门面，内部把节点、目录、传输委托给三个 Service。Gateway 按 Feature 收窄能力面；`TaskTransferGateway` 的启动/控制入口保留默认转发实现并支持继承替换，其他 Gateway 仍是具体 QObject 适配器。

### Linux 服务端

```text
handleClient
  -> safeRecvLine / 命令拆分
  -> filelist / fileput / filesave 分支
  -> resolveServerPath
  -> 文件读写、偏移和长度校验
  -> send/recv 返回结果
```

服务端路径越界、socket 断开、短读写和命令格式错误必须由 `server.cpp` 自己拒绝；Windows 客户端 Release 构建不能替代 Linux 服务端构建和协议样本。

## 六、验证矩阵

| 内容                                                 | 当前证据                                              | 状态                         |
| -------------------------------------------------- | ------------------------------------------------- | -------------------------- |
| qmake、uic、moc、Release 链接                           | `build/final-verify`                              | `Verified`                 |
| 启动窗口和上传页截图                                         | `build/final-verify/ui-smoke.png` 及历史切片截图         | `Verified`                 |
| 9 个 `.ui` 和 `resource.qrc` 与原项目一致                  | 哈希比较记录                                            | `Verified`                 |
| 任务/节点/目录/创建 Gateway 代码可达                           | 当前源代码和构建                                          | `Implemented` + `Verified` |
| 节点真实新增、重启、在线状态                                     | 无当前操作记录                                           | `Unverified`               |
| Qt 客户端 native socket 上传/断点下载 -> Linux `server.cpp` | WSL Ubuntu-D 当前 revision；文件大小和完整字节校验              | `Verified`（不覆盖 UI/任务层控制）   |
| Qt UI 目录、任务层上传/下载、暂停/恢复/取消、网络抖动                    | 尚无完整 UI 操作记录和慢网络注入                                | `Unverified`               |
| QtTest 矩阵和文件类型策略 | `tools/run-qt-test-matrix.ps1`，21 passed、0 failed；`DirectoryFileTypePolicy` 覆盖全部筛选族 | `Verified` |
| TaskManager 状态与请求映射 QtTest/fake Gateway            | `build/task-manager-state-test`，6 passed、0 failed | `Verified`                 |

## 七、推荐阅读顺序

1. `src/app/main.cpp` 和 `HomeWidge::HomeWidge`：先理解组合根。
2. `Database::Initialize` / `GetAllNodes` 与 `NodeModule::loadNodesFromDatabase`：理解节点持久化。
3. `DirectoryPageController::navigateDirectoryTab`、`FileBrowser::loadFileList`：理解异步目录回流。
4. `UploadModule::createUploadTasks`、`DownloadModule::enqueueDownloadTasks`、`TaskCreationGateway`：理解任务创建边界。
5. `TaskManager::add*Task`、`startTask`、`updateTaskStatusAtomically`：理解状态和偏移。
6. `TaskTransferGateway`、`NetWork`、`TransferService`、`TransferControlState`、`TransferProtocolClient`：理解网络线程、控制状态和协议原语。
7. `server.cpp::handleClient`、`resolveServerPath`、`handleFilePut`、`handleFileSave`：理解协议和失败路径。

每条主线都要能回答：谁触发、携带什么 ID、谁拥有对象、状态在哪里变、失败时谁保持旧状态、为什么当前边界没有继续抽象。

## 八、当前限制与下一模块入口

* `HomeWidge` 的信号连接已经按业务域分组，但页面统计、弹窗和目录树仍属于组合根；暂不直接搬到新 QObject。

* `UploadModule`/`DownloadModule` 仍混合 QWidget、QMessageBox 和前置校验；下一步先为结果对象或 fake gateway 增加测试保护。

* `TransferService` 仍包含较大的 socket/队列和文件 I/O 实现；传输状态码已下沉到 `TransferTypes.h`，协议原语已由 `TransferProtocolClient` 承担，暂停/取消状态已由 `TransferControlState` 承担，core -> feature 反向编译依赖已归零。

* `TaskTransferGateway` 仍是具体 QObject 适配器，但启动/控制方法已经可以由进程内 fake 覆盖；`NodeGateway`、`DirectoryGateway` 等仍未抽成可替换接口。

* 下一模块入口：为目录慢网络销毁竞态、Home UI 编排和 TransferService 队列执行器补确定性测试；真实 UI 端到端仍需分阶段联调。
