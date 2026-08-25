# Qt Cloud Backup System

一个 Windows 优先的 Qt Widgets 云备份客户端，配套一个可在 Linux/WSL 中运行的 POSIX Socket 服务端。项目围绕“节点管理、远程目录浏览、上传/下载任务和可恢复传输”组织，重点展示 C++/Qt 客户端、TCP 协议、并发任务和跨平台边界。

> Qt 6.8.3 · C++17 · Qt Widgets · SQLite · qmake · QThreadPool · POSIX Socket · Linux/WSL

## 项目流程

```text
新增/加载节点 -> 检查在线状态 -> 浏览远程目录
             -> 创建上传/下载任务 -> 异步传输与进度回流
             -> 暂停/恢复/取消 -> 完成或失败反馈
```

客户端负责 UI、节点数据和任务展示；Linux 服务端负责受控存储根目录下的目录读取、上传、下载和偏移续传。服务端不包含账号系统、TLS 或公网部署能力，只适合本机、WSL 或明确受控网络演示。

## 工程结构

| 目录 | 职责 |
| --- | --- |
| `src/app/` | Qt 应用入口、主题加载和主窗口启动 |
| `src/ui/shell/` | 主窗口、标题栏和跨模块信号装配 |
| `src/features/nodes/` | 节点页面、节点 CRUD 和在线状态交互 |
| `src/features/directory/` | 远程目录读取、路径导航和文件选择 |
| `src/features/transfers/` | 上传/下载输入、任务队列、状态和列表展示 |
| `src/core/network/` | 节点、目录、协议、传输和控制状态 |
| `src/core/persistence/` | SQLite 节点持久化 |
| `server/server.cpp` | Linux/WSL 独立 Socket 服务端 |
| `tests/` | QtTest、协议 Smoke 和客户端 native 传输测试 |
| `docs/` | 架构、验证边界和面试走读材料 |

## 关键设计

- **窄能力 Gateway**：页面通过 `NodeGateway`、`DirectoryGateway`、`TaskCreationGateway` 和 `TaskTransferGateway` 访问业务能力，保留 `NetWork` 作为兼容装配入口。
- **任务状态集中管理**：`TaskManager` 维护 `Waiting -> Running -> Paused -> Completed/Failed/Canceled` 状态和进度快照。
- **异步传输**：`TransferService` 使用队列和 `QThreadPool` 执行 Socket 传输，进度、状态和错误通过信号回到 Qt UI。
- **偏移续传**：上传和下载请求包含起始偏移；服务端在受控根目录内根据已有文件长度协商续传位置。
- **路径边界**：服务端对命令长度、参数解析和根目录逃逸做拒绝处理；协议 Smoke 覆盖 `../` 样例。
- **SQLite 节点数据**：客户端启动时加载节点，新增、修改和删除同步到 `nodes` 表。任务历史和完整重启恢复尚未形成闭环，不能据此宣传为已完成能力。

## Windows 客户端构建

使用 Qt Creator 打开根目录 `CloudBackupSystem.pro`，选择 Qt 6.8.x MinGW Kit。命令行示例：

```powershell
qmake CloudBackupSystem.pro
mingw32-make -j4
```

构建输出应放在仓库外的构建目录；不要把 `build/`、`debug/`、`release/`、`Makefile` 或 Qt Creator 用户配置提交到 Git。

## Linux/WSL 服务端

服务端源码位于 [`server/server.cpp`](server/server.cpp)，启动方式和协议命令见 [`server/README.md`](server/README.md)。最小流程：

```bash
g++ -std=c++17 -O2 -pthread server/server.cpp -o backup_server
./backup_server /tmp/cloud-backup-root
```

默认从 `10000` 端口开始监听；端口占用时按实现向后尝试。WSL 与 Windows 客户端联调时，使用 WSL 可达地址，并把存储根目录限定为专用演示目录。

## 验证边界

仓库保留的验证记录见 [`docs/verification.md`](docs/verification.md)：包括 QtTest 矩阵、客户端 Release/启动、Linux 服务端协议 Smoke，以及客户端 native 上传/断点下载证据。节点、目录和任务页的完整 UI 流程仍需要按照 [`docs/media/README.md`](docs/media/README.md) 重新录制截图/视频后再作为演示证据。

## 演示素材

截图命名、脱敏要求和两个录屏脚本见 [`docs/media/README.md`](docs/media/README.md)。公开仓库只放源码、测试、必要资源和文档；个人路径、运行数据库、密码、密钥和构建产物不应上传。

## 归属与限制

这是个人项目。Codex、Trae 等工具仅用于代码检索、调试、测试复核和文档整理；项目能力以当前仓库代码、可复现验证和开发者能否解释调用链、状态变化、失败处理与取舍为准。

协议服务端没有认证、TLS、设备授权和生产级访问控制，不应直接暴露到公网。
