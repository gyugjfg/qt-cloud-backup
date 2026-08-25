# Qt 云备份系统：求职证据包

> 证据窗口：2026-07-24。本文是公开仓库中的求职证据摘要，不包含本地绝对路径；具体验证命令和当前边界以仓库内 `docs/verification.md` 为准。

## 1. 项目定位

- **项目名**：Qt 云备份系统（CloudBackupSystem）
- **目标岗位**：Qt/C++ 客户端开发、C++ 应用开发实习
- **一句话问题**：桌面客户端管理云节点、浏览远程目录，并把上传/下载操作转换为可观察、可控制的传输任务。
- **技术栈**：C++17、Qt 6.8.3 Widgets、qmake、Qt Network、Qt SQL/SQLite、MinGW；独立 Linux `server.cpp` 使用 socket 提供目录和文件协议。
- **当前定位**：这是一个可构建的 Qt 客户端结构化副本，重点是保留原 UI/协议入口的同时降低模块之间的直接依赖，并补齐可解释的架构和求职证据。不能称为经过完整端到端验收的生产级产品。
- **贡献边界**：当前副本包含 AI 协助的目录整理、Gateway 窄端口、依赖收窄和文档工作。只有在本人逐函数走读并能解释触发、调用、状态、失败和取舍后，才把相应内容作为个人面试 ownership；不能把未复验的服务端联调或 AI 自动完成内容表述为独立完成。

## 2. 证据等级

| 等级 | 含义 | 当前使用规则 |
| --- | --- | --- |
| `Implemented` | 当前生产代码中存在可达实现 | 可以描述代码职责，但不等于运行成功 |
| `Verified` | 当前副本有构建、启动、截图或可复现产物 | 简历技术结果优先使用这一等级 |
| `Understood` | 本人能独立说明调用链、状态、失败处理和设计取舍 | 面试 ownership 必须达到这一等级 |
| `Unverified` | 只在文档、旧日志、预留接口或未联调路径中出现 | 不作为完成性结论或强指标使用 |

## 3. 证据台账

| 可说的主张 | 仓库证据 | 等级 | 可信度/掌握要求 | 缺失证据 |
| --- | --- | --- | --- | --- |
| 客户端按 `app/ui/features/core` 组织，qmake 清单集中在 `config/sources.pri` | `src/app/main.cpp`、`src/ui/shell/`、`src/features/`、`src/core/`、`config/sources.pri` | `Implemented` + `Verified` | 高；需能说明组合根为何仍在 `HomeWidge` | 无结构性缺口；仍需理解跨模块信号 |
| 保留原 Designer 页面和资源入口 | 9 个 `.ui` 位于 `src/ui/shell`、`src/features/*`；`resource.qrc`；`docs/testing.md` 的哈希记录 | `Implemented` + `Verified` | 高；只能说“结构/UI 资源保持”，不能说所有业务已回归 | 逻辑切片后的全量 UI 操作回归 |
| 任务管理维护上传/下载任务、状态转换、进度和偏移 | `src/features/transfers/TaskManager.h/.cpp`：`add*Task`、`startTask`、`updateTaskProgress`、`updateTaskStatusAtomically`、`sync*TransferredBytesLocked`；`build/task-manager-state-test`、`build/transfer-request-policy` | `Implemented` + `Verified` | 中；本人必须能解释 `QMap`、`QMutex` 和状态合法转换 | Qt UI 任务层暂停/恢复/取消和完整业务回流证据 |
| 任务传输通过窄端口连接网络服务 | `TaskTransferGateway.*`、`TransferTypes.h`、`TaskManager.*`、`build/task-manager-state-test`、`build/unified-transfer-service-linux-e2e` | `Implemented` + `Verified` | 中；需能画出 `TaskManager -> Gateway -> NetWork -> TransferService` | Qt UI 任务层回流、服务端异常和网络抖动证据 |
| 节点能力、目录能力与任务页节点查询已分出 Gateway | `src/core/network/NodeGateway.*`、`DirectoryGateway.*`；`TaskNodeNameGateway.*`；`docs/dependency-map.md`；Gateway contract QtTest | `Implemented` + `Verified` | 中；需能说明 Gateway 是具体适配器而非纯接口 | 真实节点和目录 UI 业务链 |
| 上传/下载输入模块只通过任务创建端口入队 | `TaskCreationGateway.*`、`UploadModule.*`、`DownloadModule.*`；当前 revision 的 `build/final-verify` | `Implemented` + `Verified` | 中；需解释为何先收窄创建能力而不动 UI | 真实上传/下载业务链仍未复验 |
| SQLite 节点初始化与 CRUD 已实现 | `src/core/persistence/Database.*`：`Initialize`、`CreateTables`、`AddNode`、`GetAllNodes`、`UpdateNode`、`DeleteNode` | `Implemented` + `Verified`（客户端编译） | 中；需说明同步 SQLite 和 `files/tasks` 预留边界 | 新增、重启加载、删除的操作记录 |
| 传输层包含异步队列、进度、暂停/取消和断点偏移逻辑 | `src/core/network/TransferService.*`：`startTransferAsync`、`processTaskQueue`、`file*Resumable`、`controlTransfer`；TransferService QtTest、Linux native E2E | `Implemented` + `Verified` | 低到中；必须逐段走读 socket、线程和失败分支 | Qt UI 任务层控制和真实网络抖动 |
| 独立服务端有路径规范化、目录、上传、下载命令分支 | `server.cpp`：`resolveServerPath`、`handleFileList`、`handleFilePut`、`handleFileSave`、`handleClient`；Linux protocol Smoke | `Implemented` + `Verified` | 低；不能只凭客户端编译宣称服务端可用 | UI 到服务端完整链路和异常注入 |
| 当前结构切片有可重复构建和启动截图 | `docs/testing.md`；`build/final-verify/release/CloudBackupSystem.exe`；`build/final-verify/ui-smoke.png` | `Verified` | 高；命令、Qt 版本和产物路径可复查 | 业务操作级自动化测试 |
| 完整任务持久化/重启恢复、端到端上传下载、QtTest 覆盖 | `Database` 的 `tasks` 表接口、旧日志或规划文档 | `Unverified` | 不应写进简历完成项 | 先补最小 fake gateway 与真实联调记录 |

### 可复现的当前客户端证据

在 Windows Qt 6.8.3 MinGW 环境中，Release 构建命令为：

```powershell
qmake CloudBackupSystem.pro -spec win32-g++ CONFIG+=release
mingw32-make -j2
```

已保存的启动产物包括 `build/final-verify/release/CloudBackupSystem.exe` 和 `build/final-verify/ui-smoke.png`；21 个 QtTest、Linux protocol Smoke 和客户端 native 上传/断点下载的命令、目录和限制见 [`docs/testing.md`](../testing.md)。这些产物证明代码和独立传输链路可复现，不替代节点、目录和任务 UI 的手工验收。

## 4. 简历素材

以下 bullet 只能在本人完成对应“掌握要求”后使用；括号中的状态不要直接放进简历，可用于面试准备。

1. **Qt Widgets/C++17 云备份客户端结构化**：按 `app/ui/features/core` 重整源码和 qmake 清单，保留 9 个 Designer 页面与 `resource.qrc`，并通过 Qt 6.8.3 MinGW Release 构建和窗口启动冒烟。（`Implemented` + `Verified`）
2. **能力边界收窄**：围绕任务传输、节点、目录和任务创建引入 Gateway 与共享传输 DTO，使 Feature 层不再散落依赖 `NetWork` 宽门面；调用链和迁移范围记录在依赖地图中。（已实现；真实业务回归仍需补）
3. **任务状态与传输协调**：在 `TaskManager` 中维护上传/下载任务快照、状态合法转换、进度回流和断点偏移同步，并用 `QMutex` 保护运行时任务集合。（代码实现存在；不要声称性能指标）
4. **桌面端持久化与网络分层**：实现 SQLite 节点初始化/CRUD，并将目录、节点、传输服务分别收口到 `NodeService`、`DirectoryService`、`TransferService`，由 `NetWork` 保持兼容入口。（节点/服务端运行证据待补）
5. **工程化表达**：补充全量代码清单、实际依赖地图、模块走读、测试矩阵和职业证据台账，使项目可按调用链复述，而不是只展示 UI 截图。（文档产物已生成）

### 不建议写进简历的句子

- “已完成高可靠、企业级云备份系统”或任何没有压测数字的性能结论。
- “上传下载、断点续传、暂停恢复全部通过 UI 测试”。当前独立传输测试和 Linux 上传/断点下载已通过，但 UI 任务层和慢网络流程仍需手工证据。
- “任务重启后自动恢复”或“服务端安全防越界已验证”。数据库预留接口和路径函数不等于端到端验收。

## 5. 60 秒项目介绍

我做的是一个 Qt Widgets/C++17 云备份客户端，解决的是在桌面端管理多个云节点、浏览远程目录，并把上传下载变成可查看进度、可暂停和取消的任务。项目入口在 `src/app/main.cpp`，主窗口 `HomeWidge` 负责组合页面；节点、目录、传输分别下沉到 `core/network`，任务状态在 `features/transfers/TaskManager` 维护，SQLite 负责节点数据，独立的 `server.cpp` 提供 socket 协议。整理过程中我保持原来的 `.ui` 和资源入口不变，只把任务、节点、目录能力通过 Gateway 收窄，减少页面对 `NetWork` 宽门面的直接耦合。当前副本已用 Qt 6.8.3 MinGW Release 构建，21 个 QtTest、Linux 协议 Smoke 和客户端 native 上传/断点下载已有证据；节点、目录和任务 UI 的完整流程仍以手工截图、日志和 hash 为准。

## 6. 3 分钟项目介绍

用户先在节点页新增或加载节点，节点信息由 `Database` 保存，在线检查通过 `NodeGateway -> NetWork -> NodeService`。进入下载页后，`FileBrowser` 经 `DirectoryGateway` 读取远程目录，用户选择文件和本地保存路径；`DownloadModule` 做节点/路径/覆盖确认，再通过 `TaskCreationGateway` 创建任务。`TaskManager` 生成快照，根据任务类型构造 `NetworkTransferRequest`，经 `TaskTransferGateway` 交给 `NetWork` 和 `TransferService` 的异步队列。传输层按 socket 结果发出进度、状态和错误信号，任务管理器再更新状态，任务页负责列表和按钮展示。

我保留了 `NetWork` 作为兼容装配入口，而不是一次性改成全新的纯虚接口，原因是这次目标是降低耦合且不改变既有 UI、协议和调用语义；Gateway 先提供窄能力面，fake Gateway 和状态/请求 QtTest 已建立。一个重要失败路径是节点不可用或传输请求校验失败：模块应在任务创建前给出反馈，传输层则发出任务错误而不是让 UI 直接操作 socket。当前限制是 `HomeWidge` 仍然承担组合根和跨页面信号桥接，`TransferService` 仍有较大实现，Qt UI 目录/任务流程、真实网络抖动和服务端异常注入仍没有绑定到当前提交。传输状态码已下沉到 `TransferTypes.h`，网络层不再编译依赖 `TaskManager.h`。下一步优先完成手工 UI 业务链证据，并把能解释的调用链整理成面试回答。

## 7. 面试回答底线

- 能独立画出：`main -> HomeWidge -> feature controller/module -> Gateway -> NetWork -> Service -> server`。
- 能解释一次任务从输入到完成的对象身份：`taskId`、`nodeId`、文件路径、偏移和状态。
- 能说清暂停、取消、失败不是同一个状态，以及信号如何回到 UI。
- 能承认 `files/tasks` 完整恢复、Qt UI 全流程和网络抖动仍缺证据；Linux 协议 Smoke 和客户端 native 上传/断点下载已有当前记录，不用旧日志替代当前验证。
- 能说明 Gateway 是兼容适配器，当前没有为所有模块提供可替换纯接口。
