# 实际依赖地图

理想依赖方向不能替代实际 include。下表保留初始扫描得到的粗粒度基线；后续切片的当前事实补在表后，避免把历史约数当成现状。

| 关系 | 约数 | 说明 |
| --- | ---: | --- |
| UI -> Feature | 30 | 主窗口和页面仍直接装配/连接多个模块。 |
| Feature -> Feature | 47 | Module、Controller、Task 页面之间存在直接协作。 |
| Feature -> Core | 9 | 节点、目录、任务模块直接依赖网络/持久化。 |
| UI -> Core | 2 | 主窗口仍直接持有网络/数据库。 |
| Core -> Feature | 0 | 传输状态码已下沉到 `TransferTypes.h`，网络实现不再包含任务模块。 |

当前静态事实：客户端 76 个 `.cpp/.h/.ui` 文件中，直接 `#include "NetWork.h"` 的文件只剩 5 个：`HomeWidge.h`、`NetWork.cpp`、`TaskTransferGateway.cpp`、`NodeGateway.cpp` 和 `DirectoryGateway.cpp`。Feature 的节点、目录、任务模块均通过能力 Gateway 或共享 DTO 进入网络层；`TaskCreationGateway` 只 include `TaskManager.h`，不接触网络门面。

本轮进一步移除了 `TransferService.cpp -> TaskManager.h` 的反向包含：传输层只使用 `NetworkTransferStatus`，任务层通过类型别名复用同一状态契约，编译依赖方向回到 `features -> core`。

## 当前重点耦合点

- `HomeWidge.cpp`：对象创建、页面路由、跨模块连接、拖拽上传、任务统计和错误反馈集中在一个类。
- `TaskManager`：已通过 `TaskTransferGateway` 隔离任务能力面，并直接使用 `core/network/TransferTypes.h` 的共享请求 DTO。
- `TaskModule`：已改为通过 `TaskNodeNameGateway` 查询节点名称，Gateway 内部仍是兼容适配器。
- `UploadModule` / `DownloadModule`：节点校验已通过 `NodeGateway` 收窄，任务创建已通过 `TaskCreationGateway` 收口；控件读取、弹窗和前置校验仍混在模块内。
- `TaskCreationGateway`：只暴露上传/下载任务创建入口，内部借用 `TaskManager`；不承担状态流转、启动、暂停、恢复或删除。
- `NodeModule`：数据库、节点 Gateway、对话框和列表控件同时出现，网络宽门面已移出。
- `DirectoryPageController`：多个 QWidget、FileBrowser、DirectoryGateway、NodeGateway 和 DirectoryModule 同时被持有；宽门面已移出。

## 目标方向

```text
CompositionRoot
  -> UI bridges/controllers
    -> feature use-case ports + DTOs
      -> core services/adapters
        -> Qt network / SQLite / server protocol
```

每次只降低一个箭头的宽度，并保留旧功能入口直到对应回归通过。不要为了目录整齐而创建没有真实职责的 Manager、Repository 或 Service。
