# 求职证据台账

完整材料入口：

- [求职证据包](evidence-pack.md)：简历边界、证据等级、60 秒/3 分钟介绍和可诚实使用的 bullet。
- [面试走读与学习地图](interview-study-map.md)：按调用链安排必读函数、失败路径和复习问题。

本页保留为短版索引；重要主张以完整证据包为准。

| 可说的主张 | 代码证据 | 当前状态 | 还需掌握或验证 |
| --- | --- | --- | --- |
| Qt Widgets/C++17 客户端按节点、目录、传输、任务分工 | `src/features/*`、`src/ui/shell/*` | Implemented | 能说清模块调用顺序。 |
| `TaskManager` 管理运行时任务状态和进度回流 | `src/features/transfers/TaskManager.*` | Implemented | 暂停、取消、失败的差别与竞态。 |
| 上传/下载通过 `TaskCreationGateway` 创建任务，任务控制仍归 `TaskManager` | `src/features/transfers/TaskCreationGateway.*`、`UploadModule.*`、`DownloadModule.*`、`build/final-verify` | Implemented/Verified | 能解释创建与启动为什么分开；真实上传/下载仍未复验。 |
| `HomeWidge` 跨模块连接按任务/目录/节点/传输分组 | `HomeWidge::bind*Signals`、`docs/development/archive/2026-07-20-home-signal-routing.md`、`build/final-verify` | Implemented/Verified | 能说明仍是组合根，不能称为独立 Presenter。 |
| 后台传输与信号槽回传 UI | `src/core/network/TransferService.*` | Implemented | 不把代码存在扩展成性能指标。 |
| SQLite 节点持久化 | `src/core/persistence/Database.*` | Implemented | 新增节点、重启、加载的原生 UI 实录。 |
| 服务端协议和偏移续传 | `server.cpp`、`src/core/network/*`、Linux protocol Smoke、`build/unified-transfer-service-linux-e2e` | Implemented/Verified | 仍需把 Qt UI 任务层和异常注入证据绑定到当前 revision。 |
| 完整任务重启恢复 | `tasks` 表和部分接口 | Unverified/预留 | 不写进简历完成项。 |

必须走读：`TaskManager` 的任务创建/启动/控制函数、`TransferService` 的传输和控制函数、`NetWork` 的转发函数、`Database` 的节点 CRUD、`server.cpp` 的命令分发和路径规范化。
