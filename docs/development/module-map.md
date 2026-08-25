# 模块地图：按真实职责拆分

本项目不把“5 个模块”当作固定模板。当前代码的边界由用户流程、数据所有权、线程边界和验证方式决定，因此采用 7 个实际模块。模块号只用于阅读顺序，不代表目录必须一一对应。

## 当前模块

| 模块 | 用户问题 | 生产代码边界 | 主要数据/状态 | 主要验证 |
| --- | --- | --- | --- | --- |
| 00 全量清单 | 项目有哪些文件、依赖和热点？ | `src/**`、`server.cpp`、`config/sources.pri` | 文件归属、直接 include、热点规模 | 静态清单、依赖扫描 |
| 01 应用组合与 UI 壳 | 程序如何启动，页面如何装配？ | `src/app`、`src/ui/shell`、`HomeWidge`、`TitleBar`、纯展示/错误/终态策略 | QObject parent、页面索引、全局 UI 反馈、下载树选择态、错误去重键、终态判断 | qmake/Release、启动截图、Home 纯策略 QtTest |
| 02 节点与持久化 | 节点如何新增、加载、修改、删除和检查在线？ | `features/nodes`、`core/persistence`、`NodeGateway` | `nodeId`、SQLite 节点表、内存节点缓存 | 客户端构建；真实节点操作待补 |
| 03 目录与文件浏览 | 如何按节点和路径浏览远程文件？ | `features/directory`、`DirectoryGateway`、`DirectoryService`、`DirectoryPathNavigation`、`DirectorySelectionPolicy` | 当前路径、请求序号、`NetworkFileInfo`、面包屑目标路径、同步取消令牌、对话框结果 | 路径/选择结果 QtTest、慢 socket 生命周期 QtTest、构建/截图；服务端目录联调待补 |
| 04 上传/下载入口 | 如何从 UI 输入生成任务？ | `UploadController/Module`、`DownloadController/Module`、`TaskCreationGateway` | 文件路径、保存路径、覆盖决策、`taskId` | Release；文件 hash/真实传输待补 |
| 05 任务运行时与控制 | 任务如何启动、暂停、恢复、取消和更新状态？ | `TaskManager`、`TaskModule`、`TaskListController`、`TaskTransferGateway`、`TaskPresentationPolicy` | `QMap` 快照、状态转换、偏移、进度、展示文案 | 状态/请求映射/展示策略 QtTest；真实端到端待补 |
| 06 客户端网络服务 | 节点、目录和传输如何复用连接、线程和协议入口？ | `NetWork`、`NodeService`、`DirectoryService`、`TransferService`、`TransferProtocolClient`、`TransferControlState`、`TransferRequestPolicy`、共享 DTO | socket、队列锁、控制状态、协议原语、恢复偏移、信号回流 | 客户端 Release、请求/队列/控制 QtTest、慢网络生命周期 QtTest、Linux native E2E |
| 07 Linux 服务端协议 | 服务端如何解析命令、限制路径并传输文件？ | `server.cpp` 独立进程 | 行协议、路径、偏移、文件读写 | Linux 构建、协议样本和越界测试待补 |

对应记录：

- 01：[应用组合与 UI 壳](2026-07-20-module-01-app-composition.md)
- 02：[节点管理与 SQLite 持久化](2026-07-20-module-02-node-persistence.md)
- 03：[目录与文件浏览](2026-07-20-module-03-directory-browser.md)
- 04：[上传、下载与任务页面](2026-07-20-module-04-transfer-task-ui.md)
- 05：[任务运行时与传输控制](2026-07-20-module-05-task-runtime.md)
- 06：[客户端网络服务](2026-07-20-module-06-client-network-services.md)
- 07：[Linux 服务端协议](2026-07-20-module-07-server-protocol.md)

新增边界记录：

- [HomeWidge 任务错误反馈规则](archive/2026-07-25-home-task-error-policy.md)
- [服务异步慢网络生命周期](archive/2026-07-26-service-async-slow-network.md)
- [TransferService 请求规则](archive/2026-07-26-transfer-request-policy.md)
- [分层依赖自动闸门](archive/2026-07-26-layer-dependency-guard.md)
- [TaskModule 任务展示策略](archive/2026-07-27-task-presentation-policy.md)
- [目录选择结果策略](archive/2026-07-27-directory-selection-policy.md)
- [HomeWidge 任务终态策略](archive/2026-07-27-home-task-status-policy.md)

## 依赖方向

```text
01 UI 壳
  -> 02 节点与持久化
  -> 03 目录浏览
  -> 04 上传/下载入口
  -> 05 任务运行时
  -> 06 客户端网络服务
  -> 07 Linux 服务端协议
```

实际例外：`Database` 由组合根创建并被节点模块借用；`NetWork` 仍是兼容门面，Gateway 仍以具体 QObject 为默认适配器，其中 `TaskTransferGateway` 的启动/控制入口可被测试替身覆盖。传输状态码已经下沉到 `TransferTypes.h`，协议原语和控制状态分别由 `TransferProtocolClient`、`TransferControlState` 承担，`core/network` 不再包含 `features/transfers/TaskManager.h`。

四个大类的实现文件已经按职责分片：`HomeWidgeComposition.cpp`/`HomeWidgeRuntimeFeedback.cpp`、`DirectoryPageControllerDialogs.cpp`、`TransferServiceFileTransfer.cpp`、`TaskModuleScheduling.cpp`。这只是编译单元和阅读边界整理，不新增用户可见功能，也不改变原类的公开接口。

## 每个模块必须交付的记录

每个模块对应一份 dated record，至少包含：

1. 模块目标、用户价值和明确不做项；
2. 文件清单、输入/输出和依赖方向；
3. 类成员、公开/私有函数、信号槽、所有权和线程；
4. 用户触发到核心服务的逐步调用链；
5. 状态、持久化、错误回流和失败保持不变量；
6. 2-5 个必须从头读到尾的函数及面试问题；
7. 真实构建、QtTest、手工验收和未覆盖范围；
8. `Implemented`、`Verified`、`Understood`、`Unverified` 状态；
9. 当前理解记录和下一模块入口。

## 历史记录映射

此前的 `module-01-task-transfer-gateway`、`module-02-app-shell-and-nodes`、`module-03-directory-browser`、`module-04-transfer-task-ui`、`module-05-core-and-server` 是早期切片记录，不删除历史事实。新的 7 模块地图把它们重新归入 01、03、04、05、06/07；后续新增记录按本地图的真实模块命名。
