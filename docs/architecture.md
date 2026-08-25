# 架构与调用链

这份文件保留为文档入口；完整的用户触发、对象职责、状态/线程/失败回流和验证矩阵见：

- [模块总览：架构与业务调用链](development/2026-07-20-architecture-and-call-chains.md)
- [实际依赖地图](dependency-map.md)
- [模块开发索引](development/README.md)

当前 revision 的高层方向：

```text
用户操作 -> HomeWidge/Controller -> Feature Module
        -> TaskCreationGateway / NodeGateway / DirectoryGateway / TaskTransferGateway
        -> NetWork -> NodeService / DirectoryService / TransferService
        -> SQLite / Qt Network / Linux server.cpp
```

这里的箭头不是“所有功能都已端到端通过”的声明。客户端 Release 构建、窗口启动、`.ui`/资源哈希、21 个 QtTest、Linux 服务端协议 Smoke 和客户端 native 上传/断点下载已验证；节点真实操作、目录/任务 UI 全流程、暂停恢复的 UI 证据和网络抖动仍按证据矩阵标记为 `Unverified`。
