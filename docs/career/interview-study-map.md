# Qt 云备份系统：面试走读与学习地图

这份地图用于把项目讲清楚，而不是背文件名。每条主线都要能回答：触发输入、调用顺序、内存/持久化状态、失败处理、为什么这样分层。完成一条主线的五项回答后，才把对应 claim 从 `Implemented` 提升到 `Understood`。

## 1. 推荐走读顺序

```text
应用启动
  -> 节点持久化与在线检查
  -> 目录读取与路径导航
  -> 上传/下载创建任务
  -> TaskManager 状态与偏移
  -> TransferService socket/线程/控制
  -> server.cpp 命令分发与路径规则
```

先理解对象边界，再读大文件的失败分支。不要从 `HomeWidge.cpp` 第一行顺读到最后一行。

## 2. 主线学习表

| 主线 | 入口与调用链 | 必须讲清的状态/失败 | 当前证据 |
| --- | --- | --- | --- |
| 启动与装配 | `main` -> `HomeWidge::HomeWidge` -> `bind*Signals` -> `GuiInit/EventInit` | QObject parent 所有权、页面控件借用关系、组合根为什么仍偏大 | `build/final-verify` Release、启动截图；真实页面操作未复验 |
| 节点保存/加载 | `NodeModule::loadNodesFromDatabase` -> `Database::GetAllNodes`；增删改经 `NodeGateway` | SQLite 初始化失败、节点 ID 一致性、同步调用是否阻塞 UI | 代码实现；真实新增/重启加载未复验 |
| 在线检查 | `NodeModule` -> `NodeGateway::checkNodeStatus` -> `NodeService::checkNodeStatus` | 已连接 socket 的复用、异步检查回调线程、失败如何反馈 | 代码实现；真实网络状态未复验 |
| 目录读取 | `FileBrowser`/`DirectoryPageController` -> `DirectoryGateway` -> `DirectoryService::getFileInfoList` | 当前路径、空列表、异步刷新、UI 线程更新 | 目录切片构建/截图；服务端联调未复验 |
| 上传创建 | `UploadController` -> `UploadModule::createUploadTasks` -> `TaskCreationGateway::createUploadTask` -> `TaskManager::addUploadTask` | 文件选择、节点可用性、taskId 生成、重复/空路径 | TaskCreation slice 已构建；真实上传和手工 UI 业务证据未复验 |
| 下载创建 | `DownloadController` -> `DownloadModule::enqueueDownloadTasks` -> `TaskCreationGateway::createDownloadTask` -> `TaskManager::addDownloadTask` | 远程路径、本地保存路径、覆盖确认、文件大小和目录项过滤 | 代码实现；真实下载未复验 |
| 任务启动 | `TaskModule::appendCreatedTaskAndStart` -> `TaskManager::startTask` -> `start*TaskLocked` -> `TaskTransferGateway::startTransfer` | Waiting/Running、恢复偏移、请求方向、Gateway 不拥有 NetWork | TransferRequest/TaskManager QtTest、Linux native 上传/断点下载；UI 任务链待手工验收 |
| 任务控制 | `TaskListController` -> `TaskModule::toggleTaskById` -> `TaskManager::pauseTask/cancelTask/resumeTask` -> `TaskTransferGateway::controlTransfer` | 合法状态转换、暂停 vs 取消、竞态、控制失败后的状态 | TransferControlState、TransferService fixture QtTest；UI 控制和网络抖动待手工验收 |
| 进度/完成回流 | `TransferService` signals -> `NetWork` -> `TaskTransferGateway` -> `TaskManager::updateTaskProgress/updateTaskStatus` -> `TaskModule/TaskItem` | 跨线程信号值传递、进度边界、失败/取消/完成文案 | TransferService/TaskManager QtTest 和 native E2E；完整 UI 回流待手工验收 |
| 服务端协议 | `server.cpp::handleClient` -> `handleFileList/handleFilePut/handleFileSave` | 行协议、长度校验、路径规范化、越界拒绝、socket 断开 | Linux protocol Smoke 和 Qt 客户端 native E2E 已验证；服务端异常注入待补 |

## 3. 必读函数清单

### A. 应用与组合根

1. `src/app/main.cpp::main` 与 `applyApplicationTheme`
   - **责任**：创建 QApplication、加载主题、设置窗口尺寸并启动事件循环。
   - **下游**：`HomeWidge`，Qt event loop。
   - **失败/取舍**：主题文件打开失败时保持默认样式；入口保持薄，装配留在主窗口。
   - **过关问题**：为什么不能在 `main` 里创建所有模块？`QApplication` 何时必须先于 QWidget？
2. `src/ui/shell/HomeWidge.cpp::HomeWidge`、`bindTaskSignals`、`bindDirectorySignals`、`bindNodeSignals`、`bindTransferSignals`
   - **责任**：组合对象、页面路由、跨模块信号桥接和全局反馈。
   - **下游**：所有 Feature/Controller；不应下沉业务规则。
   - **失败/取舍**：保留原连接语义，按领域分组降低构造函数阅读成本；当前仍是热点。
   - **过关问题**：哪些对象由 parent 拥有？哪些 QWidget 是借用？一个任务错误最终由谁弹窗？

### B. 节点与持久化

3. `src/core/persistence/Database.cpp::Initialize`、`CreateTables`
   - **责任**：打开 SQLite 连接并建立当前 schema/索引。
   - **下游**：`NodeModule` 的加载与 CRUD。
   - **失败/取舍**：初始化或 SQL 错误必须返回失败；`files/tasks` 接口保留但不应被说成完整恢复闭环。
   - **过关问题**：连接名/生命周期如何管理？重启加载依赖哪张表？
4. `Database.cpp::AddNode`、`UpdateNode`、`DeleteNode`、`GetAllNodes`
   - **责任**：节点增删改查和 DTO 转换。
   - **下游**：`NodeGateway`/`NodeModule`。
   - **失败/取舍**：SQL 失败返回 `false` 或空结果，不伪造成功。
   - **过关问题**：节点 ID 从哪里来？删除节点时在线 socket 如何处理？
5. `src/core/network/NodeService.cpp::checkNodeStatus`、`checkNodeStatusAsync`、`getConnection`
   - **责任**：连接复用、同步/异步在线检查和 socket 获取。
   - **下游**：`NodeGateway`、目录/上传/下载校验。
   - **失败/取舍**：连接不存在或状态非 Connected 时返回离线；异步结果用 queued callback 回到对象线程。
   - **过关问题**：为什么 UI 校验不能直接创建 QTcpSocket？线程池回调如何避免直接碰 QWidget？

### C. 目录与文件浏览

6. `src/core/network/DirectoryService.cpp::getFileInfoList`、`startFileListSync`、`changeDirectory`
   - **责任**：目录请求、当前路径和定时同步。
   - **下游**：`DirectoryGateway` -> `FileBrowser`/`DirectoryPageController`。
   - **失败/取舍**：网络失败返回空/错误路径结果；定时器状态按 nodeId 管理。
   - **过关问题**：当前路径按节点还是全局保存？空目录和网络失败在 UI 上如何区分？
7. `src/features/directory/FileBrowser.cpp::loadFileList`、`handleFileListUpdated`、选择相关函数
   - **责任**：把远程文件结果映射为列表项，并把选择结果交给下载入口。
   - **下游**：`DownloadModule`、目录页 UI。
   - **失败/取舍**：后台结果回到 UI 线程后才创建控件；不把 QWidget 传入网络线程。
   - **过关问题**：用户选中的远程路径如何保留？目录项与文件项如何过滤？

### D. 上传、下载与任务

8. `src/features/transfers/UploadModule.cpp::ensureUploadNodeSelected`、`createUploadTasks`
   - **责任**：校验节点、收集文件路径、调用创建端口并发出任务创建信号。
   - **下游**：`TaskCreationGateway` -> `TaskManager::addUploadTask`。
   - **失败/取舍**：空文件、离线节点或无创建端口时跳过/反馈；不让页面直接调用状态机。
   - **过关问题**：为什么任务创建与任务启动分成两个信号阶段？多文件上传如何保持每个 taskId？
9. `src/features/transfers/DownloadModule.cpp::ensureDownloadNodeReady`、`confirmDownloadOverwrite`、`enqueueDownloadTasks`
   - **责任**：节点和保存路径校验、覆盖确认、批量创建下载任务。
   - **下游**：`TaskCreationGateway::createDownloadTask`。
   - **失败/取舍**：目录项不入队；覆盖拒绝不创建任务。
   - **过关问题**：远程文件大小何时进入任务快照？保存路径与远程路径为什么分字段？
10. `src/features/transfers/TaskManager.cpp::addUploadTask`、`addDownloadTask`、`startTask`
    - **责任**：生成 taskId、登记任务、选择上传/下载启动路径。
    - **下游**：`startUploadTaskLocked`/`startDownloadTaskLocked` -> Gateway。
    - **失败/取舍**：不存在任务或 Gateway 不可用时不启动；偏移从当前任务状态同步。
    - **过关问题**：任务 ID 如何保证当前进程内唯一？启动失败时状态是否回滚？
11. `TaskManager.cpp::updateTaskStatusAtomically`、`isValidTransition`、`updateTaskProgress`
    - **责任**：在锁保护下更新任务状态和进度，并向 UI 发信号。
    - **下游**：`TaskModule`、`TaskItem`、主页统计。
    - **失败/取舍**：非法转换拒绝；进度和状态更新必须避免并发破坏快照。
    - **过关问题**：Paused -> Running 与 Failed -> Running 是否都允许？发信号应在锁内还是锁外？
12. `TaskManager.cpp::sync*TransferredBytesLocked`、`build*TransferRequest`
    - **责任**：计算续传偏移并构造共享请求 DTO。
    - **下游**：`TaskTransferGateway::startTransfer`。
    - **失败/取舍**：偏移不能超过总大小；请求使用 `TransferTypes.h` 而非网络宽门面类型。
    - **过关问题**：重启恢复当前依赖什么持久化？为什么现在不能声称完整任务恢复？

### E. 网络传输与服务端

13. `src/core/network/TransferService.cpp::startTransferAsync`、`processTaskQueue`、`executeTransferTask`
    - **责任**：排队、调度和选择上传/下载执行函数。
    - **下游**：`fileUploadResumable`、`fileDownloadResumable`。
    - **失败/取舍**：队列和控制状态分别加锁；阻塞 socket 工作不应阻塞 UI 线程。
    - **过关问题**：任务如何从队列进入执行？同一 taskId 的控制请求如何被观察？
14. `TransferService.cpp::fileUploadResumable`、`fileDownloadResumable`、`controlTransfer`
    - **责任**：带偏移的 socket 传输、进度和暂停/取消响应。
    - **下游**：信号 -> `NetWork` -> Gateway -> TaskManager。
    - **失败/取舍**：发送失败、服务端拒绝、暂停和取消分别发出不同结果；独立 Linux 上传/断点下载已有字节校验，UI 任务层和慢网络仍需手工证据。
    - **过关问题**：如何避免暂停后把已提交字节数算错？取消后本地半文件怎么处理？
15. `server.cpp::safeRecvLine`、`resolveServerPath`、`handleClient`、`handleFileList`、`handleFilePut`、`handleFileSave`
    - **责任**：读取命令、规范化路径、处理目录/上传/下载协议分支。
    - **下游**：Linux socket 客户端连接。
    - **失败/取舍**：长度、解析和路径越界应在服务端拒绝；Linux protocol Smoke 已绑定当前服务端构建和 `../` 拒绝样本。
    - **过关问题**：为什么路径规范化必须在服务端做？如何证明 `../` 不会逃逸根目录？

## 4. 分层问题库

对每条主线按以下顺序自测：

1. **Entry**：用户或网络事件从哪里进入？携带的 `taskId`/`nodeId`/路径是什么？
2. **Call path**：经过哪些 Controller、Module、Gateway、Service？每层为什么存在？
3. **State**：哪些字段进入 `QMap`/快照/SQLite？何时发信号？
4. **Failure**：节点离线、socket 断开、SQL 失败、路径越界和取消分别由谁处理？
5. **Tradeoff**：为什么先保留 `NetWork` 兼容入口和具体 Gateway，而不是一次性改纯接口？

## 5. 缺口分类

| 缺口 | 类型 | 对求职的影响 | 最小行动 |
| --- | --- | --- | --- |
| 本人还不能独立复述完整上传/下载调用链 | `Understanding gap` | 面试 ownership 不稳 | 按第 8-14 项逐函数读完，手写一张时序图 |
| 上传/下载真实业务链和文件 hash 未复验 | `Evidence gap` | 不能把代码实现写成端到端完成 | 使用固定小文件完成一次上传/下载并保存 hash、taskId、日志 |
| 没有 QtTest fake gateway | `Career blocker`（若岗位要求测试） | 难证明状态机和失败分支 | 先为 `TaskManager` 写状态转换最小测试，不改 UI |
| Linux server、目录、上传、下载、断点和越界未联调 | `Evidence gap` | 不能说端到端完成 | 保存命令、日志、文件 hash 和异常样本 |
| `HomeWidge` 仍是大型组合根 | `Product backlog` | 影响可维护性叙述，但不阻止当前投递 | 先保留 `bind*Signals`，以后以测试保护继续拆 |
| `Database` 的 `files/tasks` 未形成恢复闭环 | `Product backlog` | 不应承诺重启恢复 | 在面试中主动说明当前边界 |

## 6. 三个最高价值动作

1. **掌握一条完整主线**：先画上传从文件选择到服务端响应的时序图，并能说明每个失败点。
2. **补最小自动证据**：为 `TaskManager::isValidTransition`、`updateTaskStatusAtomically` 和 Gateway 不可用分支增加 QtTest/fake gateway，不改 UI。
3. **做一次真实联调记录**：用固定小文件记录节点、服务端命令、任务状态、最终 hash 和异常路径，绑定到提交和日期。
