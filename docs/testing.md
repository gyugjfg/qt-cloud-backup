# 测试与验收矩阵

## 已执行证据

| 证据 | 命令/位置 | 结果 |
| --- | --- | --- |
| 结构基线构建 | Qt 6.8.3 MinGW、`build/structural-verify` | 通过 |
| Gateway 切片构建 | Qt 6.8.3 MinGW、`build/gateway-verify` | 通过 |
| 共享传输类型构建 | Qt 6.8.3 MinGW、`build/types-verify` | 通过 |
| 节点名称 Gateway 构建 | Qt 6.8.3 MinGW、`build/node-gateway-verify` | 通过 |
| 节点能力完整切片构建 | Qt 6.8.3 MinGW、`build/node-gateway-full-verify` | 通过 |
| 目录能力完整切片构建 | Qt 6.8.3 MinGW、`build/directory-gateway-verify` | 通过 |
| 组合根头文件收窄构建 | Qt 6.8.3 MinGW、`build/shell-header-verify` | 通过（首次编译发现并补齐显式 include） |
| 任务创建 Gateway 接入构建 | Qt 6.8.3 MinGW、`build/creation-gateway-final-verify` | 通过（含 `uic`/`moc`/Release 链接） |
| 当前合并切片构建 | Qt 6.8.3 MinGW、`build/final-verify` | 通过（含 `uic`/`moc`/Release 链接） |
| 传输边界与中文注释切片构建 | Qt 6.8.3 MinGW、`build/comment-boundary-verify` | 通过（`TransferService.cpp` 不再依赖 `TaskManager.h`） |
| 传输共享契约 QtTest | Qt 6.8.3 MinGW、`build/transfer-contract-test/release/transfer_contract_test.exe` | 通过，退出码 0 |
| Linux 服务端协议 smoke | WSL Ubuntu-D、`g++ -std=c++17 -O2 -Wall -Wextra -pthread server/server.cpp` + `tests/server_protocol_smoke.py` | 通过；110843 字节上传/下载 SHA-256 一致，目录列表和路径穿越拒绝通过 |
| TaskManager 状态迁移与 fake Gateway QtTest | Qt 6.8.3 MinGW、`build/task-manager-state-test/release/task_manager_state_test.exe` | 通过，6 passed、0 failed；覆盖空 Gateway、状态边界和请求/控制映射 |
| TaskTransferGateway 可替换端口客户端构建 | Qt 6.8.3 MinGW、`build/gateway-fake-verify` | 通过，含 uic/moc/rcc、Release 链接和启动冒烟 |
| 目录选择异步生命周期守卫构建 | Qt 6.8.3 MinGW、`build/directory-lifetime-verify` | 通过，Release 链接和启动冒烟；未改 `.ui`/资源 |
| 当前架构边界 Release 构建 | Qt 6.8.3 MinGW、`build/architecture-boundary-verify` | 通过，含 uic/moc/rcc、Release 链接和启动冒烟 |
| TransferService 关闭守卫 QtTest | Qt 6.8.3 MinGW、`build/transfer-service-shutdown/release/transfer_service_shutdown_test.exe` | 通过，6 个测试函数（4 个业务用例）0 failed，退出码 0 |
| TransferService 队列调度 QtTest | Qt 6.8.3 MinGW、`build/transfer-service-queue-dispatch/release/transfer_service_shutdown_test.exe` | 通过，新增多请求排空用例；每个排队请求均回传错误和失败状态 |
| HomeDownloadSelectionPolicy QtTest | Qt 6.8.3 MinGW、`build/home-download-selection-policy/release/home-download-selection-policy-test.exe` | 通过，空集合、全选、混合/半选 3 个业务用例、0 failed |
| DirectoryPathNavigation QtTest | Qt 6.8.3 MinGW、`build/directory-path-navigation/release/directory_path_navigation_test.exe` | 通过，父路径和面包屑 2 个业务用例、0 failed |
| HomeTaskErrorPolicy QtTest | Qt 6.8.3 MinGW、`build/home-task-error-policy/release/home-task-error-policy-test.exe` | 通过，上传/下载标题和去重键 3 个业务用例、0 failed |
| TransferRequestPolicy QtTest | Qt 6.8.3 MinGW、`build/transfer-request-policy/release/transfer_request_policy_test.exe` | 通过，端点、上传偏移、下载偏移 3 个业务用例、0 failed |
| DirectoryService 慢网络生命周期 QtTest | Qt 6.8.3 MinGW、`build/service-async-slow-network/release/service_async_slow_network_test.exe` | 通过，停止同步/析构后迟到响应 2 个业务用例、0 failed |
| 分层依赖自动闸门 | PowerShell、`tools/verify-layer-dependencies.ps1` | 通过，退出码 0；未发现 core/features 反向包含 UI 层 |
| TaskPresentationPolicy QtTest | Qt 6.8.3 MinGW、`build/task-presentation-policy/release/task-presentation-policy-test.exe` | 通过，状态/终态/大小/进度文本 4 个业务用例、0 failed |
| DirectorySelectionPolicy QtTest | Qt 6.8.3 MinGW、`build/directory-selection-policy/release/directory_selection_policy_test.exe` | 通过，Accepted/节点索引/路径 2 个业务用例、0 failed |
| DirectoryFileTypePolicy QtTest | Qt 6.8.3 MinGW、`build/directory-file-type-policy/release/directory_file_type_policy_test.exe` | 通过，文本/图片/音频/视频/压缩/目录/可执行文件筛选和展示类型 3 个业务用例、0 failed；无后缀文件按 `/bin`、`/usr/bin` 等标准可执行目录做保守识别 |
| HomeTaskStatusPolicy QtTest | Qt 6.8.3 MinGW、`build/home-task-status-policy/release/home-task-status-policy-test.exe` | 通过，终态/活跃态/未知状态 3 个业务用例、0 failed |
| QtTest 一键矩阵脚本 | PowerShell、`tools/run-qt-test-matrix.ps1` | 已验证：21 个 QtTest 工程构建并运行，21 passed、0 failed；默认跳过需要外部 Linux/WSL 的 native E2E；报告位于 `build/test-matrix/<test-project>/test-report.xml` |
| 四个热点实现文件职责拆分 | Qt 6.8.3 MinGW、`build/final-verify` + QtTest 矩阵 | 通过；HomeWidge、DirectoryPageController、TransferService、TaskModule 的新旧实现单元均完成 Release 编译链接；矩阵 21 passed、0 failed，未改 `.ui`/资源/协议/schema |
| 三服务异步生命周期 QtTest | Qt 6.8.3 MinGW、`build/service-async-lifetime-test/release/service_async_lifetime_test.exe` | 通过，5 个测试函数、0 failed；覆盖 Node/Directory worker 与活动上传析构收敛 |
| Home 任务反馈摘要 QtTest | Qt 6.8.3 MinGW、`build/home-task-feedback-summary/release/home-task-feedback-summary-test.exe` | 通过，2 个业务测试、0 failed；覆盖终态计数、文件分组和空批次 |
| DirectoryLoadCoordinator QtTest | Qt 6.8.3 MinGW、`build/directory-load-coordinator/release/directory_load_coordinator_test.exe` | 通过，3 个业务测试、0 failed；覆盖 GUI 回调、旧代淘汰和销毁丢弃 |
| TransferService Qt socket fixture E2E | Qt 6.8.3 MinGW、`build/transfer-service-e2e/release/transfer_service_e2e_test.exe` | 通过，上传/下载 round-trip、慢下载暂停/恢复/取消和协议字段断言、0 failed；fixture 不替代 Linux 部署联调 |
| HomeFileRowPresentation QtTest | Qt 6.8.3 MinGW、`build/unified-home-file-row-presentation/release/home-file-row-presentation-test.exe` | 通过，2 个展示映射/大小边界用例、0 failed |
| DirectoryTabPresentation QtTest | Qt 6.8.3 MinGW、`build/unified-directory-tab-presentation/release/directory_tab_presentation_test.exe` | 通过，2 个页签标题/节点名规则用例、0 failed |
| TransferControlState QtTest | Qt 6.8.3 MinGW、`build/unified-transfer-control-state/release/transfer_control_state_test.exe` | 通过，3 个请求优先级/方向隔离/关闭快照用例、0 failed |
| Qt 客户端 -> Linux server.cpp E2E | WSL Ubuntu-D + Qt 6.8.3 MinGW、`build/unified-transfer-service-linux-e2e/release/transfer_service_linux_e2e_test.exe` | 通过，当前 TransferControlState 重构后真实上传和预写前缀后的断点下载，业务测试 1 passed、0 failed；不覆盖 UI/任务层控制 |
| NetWork 门面不变量 | Qt 6.8.3 MinGW、`build/architecture-boundary-verify` | 通过，服务转发门面构建并启动；移除不可达的自有依赖空回退 |
| uic/moc/rcc | qmake 生成过程 | 通过 |
| 启动窗口 | `build/final-verify/release/CloudBackupSystem.exe` | 通过 |
| UI 截图 | `build/gateway-verify/ui-smoke.png`、`build/node-gateway-verify/ui-smoke.png`、`build/node-gateway-full-verify/ui-smoke.png`、`build/directory-gateway-verify/ui-smoke.png`、`build/shell-header-verify/ui-smoke.png`、`build/final-verify/ui-smoke.png` | 已保存 |
| UI/资源回归 | 原项目与副本按文件名匹配的 9 个 `.ui` 和 `resource.qrc` SHA-256 | 通过，均无差异；副本仅调整源码目录布局 |

## 未覆盖的真实业务流程

| 流程 | 当前状态 | 最小验收证据 |
| --- | --- | --- |
| 节点新增、重启、加载 | 未复验 | 客户端操作记录 + SQLite 节点记录 + 截图 |
| 远程目录浏览 | 协议级已验证，UI 未复验 | Linux 服务端协议 + UI 目录进入/返回 |
| 单/多文件上传 | 协议级单文件已验证，Qt UI 未复验 | 输入文件 hash、服务端结果、任务状态截图 |
| 单/多文件下载 | 协议级单文件已验证，Qt UI 未复验 | 下载文件 hash、保存路径、任务状态截图 |
| 暂停/恢复/取消 | 未复验 | 大文件偏移、状态变化、恢复后 hash |
| 服务端异常和路径越界 | 路径越界协议级已验证，其他异常未复验 | 错误消息、拒绝结果和日志 |

任何后续逻辑解耦都必须至少重复构建、启动截图和对应业务链手工验收；当前已有共享契约和任务状态的局部 QtTest，但仍不应宣称“全功能自动回归通过”。
