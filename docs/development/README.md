# 开发与重构闸门

这个目录按“先读结论，再查证据”组织。你不需要逐个打开所有文件。

## 最短阅读路径

1. [项目走读入口](./START-HERE.md)：按用户流程理解调用链、关键函数和未验证项。
2. [模块地图](./module-map.md)：确认 7 个真实职责模块、状态所有权和依赖方向。
3. [架构与业务调用链](./2026-07-20-architecture-and-call-chains.md)：需要画流程图或准备面试时再读。
4. [测试与验收矩阵](../testing.md) 和 [验证记录](../verification.md)：只看当前 revision 的证据。

当前基线：21 个 QtTest 工程全部通过；Release 构建、启动冒烟、Linux 协议 Smoke 和客户端 native 上传/断点下载已有记录。未写入截图、日志、hash 的 UI 流程仍保持 `Unverified`。

## 模块文档

以下 8 份模块文档是稳定的“理解材料”，按 `START-HERE.md` 的流程选择性阅读：

- [模块 00：全量代码清单](./2026-07-20-module-00-code-inventory.md)
- [模块 01：应用组合与 UI 壳](./2026-07-20-module-01-app-composition.md)
- [模块 02：节点管理与 SQLite 持久化](./2026-07-20-module-02-node-persistence.md)
- [模块 03：目录与文件浏览](./2026-07-20-module-03-directory-browser.md)
- [模块 04：上传、下载与任务页面](./2026-07-20-module-04-transfer-task-ui.md)
- [模块 05：任务运行时与传输控制](./2026-07-20-module-05-task-runtime.md)
- [模块 06：客户端网络服务](./2026-07-20-module-06-client-network-services.md)
- [模块 07：Linux 服务端协议](./2026-07-20-module-07-server-protocol.md)

## 当前审计

[代码注释与耦合审计](./2026-07-20-code-comment-and-coupling-audit.md) 是当前代码结构、注释覆盖和剩余耦合问题的总览。它比历史切片更适合作为重构状态入口。

## 历史与证据

其余 dated 文档是每个小切片的目标、调用链、测试命令和证据记录，保留它们是为了复盘和面试举证，不是为了增加阅读负担。它们已移到 [archive/](./archive/README.md)，需要查某个类或某次验证时再按索引搜索。

每个切片仍遵守同一记录格式：目标/不做项、依赖方向、调用链、2-5 个走读函数、不变量、构建与测试、`Implemented`/`Verified`/`Understood`/`Unverified`。文档不能用“代码看起来合理”替代真实测试，也不能用 AI 生成的解释替代用户的 `Understood`。

项目级执行边界见 [`.codex/skills/cloud-backup-project/SKILL.md](../../.codex/skills/cloud-backup-project/SKILL.md)。
