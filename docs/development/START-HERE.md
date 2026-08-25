# 项目走读入口

这份入口解决两个问题：项目是否已经有模块开发文档，以及第一次阅读应该按什么顺序进入代码。当前项目已经有 7 个真实职责模块和多份 dated 切片记录；不要从最早的历史日志开始读。

## 先看什么

1. [模块地图](./module-map.md)：确认模块边界、状态所有权和主要验证方式。
2. [架构与业务调用链](./2026-07-20-architecture-and-call-chains.md)：按用户操作追踪对象、线程、失败回流。
3. [代码注释与耦合审计](./2026-07-20-code-comment-and-coupling-audit.md)：查看哪些边界已经实际收紧，哪些仍未验证。
4. [测试与验收矩阵](../testing.md)：区分自动测试、构建证据和必须手工完成的 UI 联调。
5. [验证记录](../verification.md)：只把当前 revision 的证据当作 Verified。

具体重构切片和历史决策已集中到 [archive/README.md](./archive/README.md)。除非要追查某个边界或准备具体面试证据，否则不需要打开归档目录中的文件。

## 按用户流程阅读

### 1. 程序启动和节点持久化

调用链：

```text
main
  -> HomeWidge::HomeWidge
  -> Database::Initialize
  -> NodeModule::loadNodesFromDatabase
  -> NodeGateway / NodeService
  -> 节点列表和下拉框
```

必须走读：`main`、`HomeWidge::HomeWidge`、`HomeWidge::DatabaseInit`、`NodeModule::loadNodesFromDatabase`、`Database::Initialize`。

关键不变量：`nodeId` 是节点、目录、任务和网络连接之间的唯一身份；数据库 CRUD 与内存网络节点写入不是一个跨存储事务。

### 2. 远程目录浏览

调用链：

```text
目录页操作
  -> DirectoryPageController
  -> FileBrowser
  -> DirectoryGateway
  -> DirectoryService / NodeService
  -> NetworkFileInfo 值快照
  -> GUI 线程填充 QTreeWidget
```

必须走读：`DirectoryPageController::navigateDirectoryTab`、`openDirectorySelectionDialog`、`FileBrowser::loadFileList`、`DirectoryService::getFileInfoList`、`DirectoryLoadCoordinator::request`。

关键不变量：worker 不触碰 QWidget；请求代际、取消令牌和 `QPointer` 只负责丢弃过期/迟到结果，不替代真实网络取消。

### 3. 上传/下载任务创建

调用链：

```text
HomeWidge 输入
  -> UploadModule / DownloadModule
  -> TaskCreationGateway
  -> TaskManager::addUploadTask / addDownloadTask
  -> TaskModule::appendCreatedTaskAndStart
```

必须走读：`UploadModule::createUploadTasks`、`DownloadModule::createDownloadTasks`、`DownloadModule::enqueueDownloadTasks`、`TaskCreationGateway`、`TaskManager::add*Task`。

关键不变量：`taskId` 在任务登记时产生；远程路径、显示文件名、本地保存路径和节点 ID 不是同一个字段；目录项不能直接进入文件下载任务。

### 4. 任务状态和控制

调用链：

```text
TaskModule
  -> TaskManager::startTask / pauseTask / resumeTask / cancelTask
  -> TaskTransferGateway
  -> NetWork
  -> TransferService
  -> taskProgressChanged / taskStatusChanged / taskError
```

必须走读：`TaskManager::startTask`、`TaskManager::updateTaskStatusAtomically`、`TaskManager::syncDownloadTransferredBytesLocked`、`TaskModule::handleTaskCompletion`、`HomeWidge::handleTaskStatusChanged`。

关键不变量：任务状态事实在 `TaskManager`；主页统计是展示投影；暂停保存恢复偏移，取消是终止语义，失败来自传输/协议错误。

### 5. 客户端传输和 Linux 服务端

调用链：

```text
TransferService
  -> TransferProtocolClient
  -> Qt socket / Linux server.cpp
  -> 文件偏移、进度、状态和错误回流
```

必须走读：`TransferService::fileUploadResumable`、`fileDownloadResumable`、`TransferService::processTaskQueue`、`TransferControlState`、`server.cpp::handleClient`、`resolveServerPath`。

关键不变量：`TransferProtocolClient` 只负责协议原语；`TransferService` 负责文件 I/O、队列和外层生命周期；Linux 服务端路径越界拒绝不能由 Windows 客户端构建代替。

## 自动验证

在 Qt 6.8.3 MinGW 环境运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run-qt-test-matrix.ps1
```

该脚本默认构建并运行所有 QtTest，但跳过需要外部 Linux/WSL 环境的 `transfer-service-linux-e2e-test.pro`。确认 Linux 服务端和环境已经准备好后，再显式运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run-qt-test-matrix.ps1 -IncludeLinuxE2E
```

最近一次默认运行结果：21 个 QtTest 工程全部通过（21 passed、0 failed）。报告写入 `build/test-matrix/<test-project>/test-report.xml`，构建产物不会进入 Git。

## 必须手工完成的证据

这些步骤不能由纯 QtTest 替代，完成后把截图、服务端日志、文件 hash 和日期写入 [验证记录](../verification.md)：

| 手工流程 | 最小证据 | 当前状态 |
| --- | --- | --- |
| UI 新增节点、重启、加载 | 节点列表截图 + SQLite 记录 | `Unverified` |
| UI 打开远程目录、进入/返回/面包屑 | Linux 服务端日志 + UI 截图 | `Unverified` |
| UI 上传/下载大文件 | 源文件和结果文件 SHA-256 + 任务截图 | `Unverified` |
| UI 暂停/恢复/取消 | 状态变化截图 + 恢复后 hash | `Unverified` |
| 真实部署网络抖动 | 服务端/客户端日志和失败回流 | `Unverified` |

## 理解验收

每条流程都要能用自己的话回答：触发点和输入是什么、调用顺序是什么、状态在哪里改变、失败后谁保持旧状态、为什么不让上一层直接操作下一层。回答不出来的部分标记为 `Understood` 待走读，不把文档阅读自动算成掌握。
