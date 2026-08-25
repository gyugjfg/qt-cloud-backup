# 当前文档入口

第一次阅读只需要按这个顺序走：`development/START-HERE.md` -> `development/module-map.md` -> `testing.md` -> `verification.md`。归档目录用于追溯具体切片，不需要逐篇阅读。

| 文档 | 用途 |
| --- | --- |
| [复制基线](baseline.md) | 来源、快照提交和安全边界。 |
| [架构与调用链](architecture.md) | 目录职责、依赖方向和上传/下载/任务链路。 |
| [架构与业务调用链详解](development/2026-07-20-architecture-and-call-chains.md) | 按用户流程说明对象、状态、线程、失败和验证证据。 |
| [验证记录](verification.md) | 构建、启动、哈希回归和未覆盖范围。 |
| [项目走读入口](development/START-HERE.md) | 不逐篇阅读开发记录时的最短理解路径。 |
| [开发闸门](development/README.md) | 后续每个重构切片的目标、范围和验收格式。 |
| [模块地图](development/module-map.md) | 按真实职责划分的 00-07 模块和历史记录映射。 |
| [全量代码审计](development/archive/2026-07-20-full-code-audit.md) | 76 个客户端文件、服务端和剩余耦合点的历史基线；当前结构以模块地图和代码注释审计为准。 |
| [代码注释与耦合审计](development/2026-07-20-code-comment-and-coupling-audit.md) | 头/源文件注释覆盖、真实职责校准和本轮未解决问题。 |
| [传输共享契约测试](development/archive/2026-07-22-transfer-contract-test.md) | QtTest 对状态码和请求默认值的最小自动保护。 |
| [求职证据台账](career/evidence-ledger.md) | 简历与面试主张的代码证据和缺口。 |

当前自动证据基线：21 个 QtTest 工程全部通过，客户端 Release/启动、Linux 服务端协议 Smoke 和 Qt 客户端 native 上传/断点下载已记录；节点、目录和任务页的完整 UI 操作仍以手工截图、日志和 hash 作为最终证据。

根目录旧文档、`项目参考文档V1/` 和 `实习投递材料/` 保留原样，需通过本目录口径校准后再引用。
