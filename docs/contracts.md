# 状态、协议与数据契约

## 任务状态

| 状态 | 语义 | 允许的用户动作 | 当前存储边界 |
| --- | --- | --- | --- |
| `Waiting` | 已创建但尚未启动 | 开始、取消 | TaskManager 内存 |
| `Running` | 传输正在执行 | 暂停、取消 | TaskManager + TransferService |
| `Paused` | 用户暂停或传输保留偏移 | 恢复、取消 | TaskManager 内存与本地文件偏移 |
| `Completed` | 当前传输成功完成 | 展示、删除 | TaskManager 内存 |
| `Failed` | 当前传输失败 | 重新处理由现有 UI 语义决定 | TaskManager 内存 |
| `Canceled` | 用户主动取消 | 展示、删除 | TaskManager 内存 |

`files`、`tasks` 表与部分 CRUD 存在，但没有接入完整的启动恢复闭环，不能把它们写成任务历史持久化。

## 传输请求

当前请求由 `core/network/TransferTypes.h` 中的 `NetworkTransferRequest` 描述，包含传输类型、路径、节点、任务 ID、线程数和起始偏移。`NetWork` 保留同名类型别名兼容旧调用方；任务层直接依赖共享 DTO，不再通过 Gateway 间接包含 `NetWork.h`。

## 信号回流

`TransferService` 发出进度、状态和错误；`NetWork` 转发；`TaskTransferGateway` 再转发给 `TaskManager`；任务层更新快照并通知页面模块。UI 不应直接修改底层传输状态。

## 任务创建

上传/下载输入模块通过 `TaskCreationGateway` 创建任务，只接收任务 ID 作为结果；该端口内部借用 `TaskManager`，不暴露状态查询、启动、暂停、恢复或删除能力。创建成功后的自动启动仍由 `TaskModule` 调用 `TaskManager` 完成。

## 数据持久化

`Database` 负责 SQLite 初始化和节点 CRUD。节点数据可以在客户端启动时加载；运行时任务仍由 `TaskManager` 管理。不要把“存在表结构”当作“完成恢复能力”。

## 节点能力

`NodeGateway` 对 Feature 暴露节点新增、删除、更新、查询和在线检查；它只借用组合根创建的 `NetWork`，不拥有网络服务，也不改变节点状态码或连接策略。

`DirectoryGateway` 对目录 Feature 暴露文件列表读取、单文件信息读取和自动刷新控制；目录回流信号只转发值对象，不携带 QWidget。路径语义、刷新间隔和服务端协议仍由原 `NetWork/DirectoryService` 保持。
