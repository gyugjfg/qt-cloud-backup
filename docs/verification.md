# 验证记录

## 已执行

| 项目 | 结果 |
| --- | --- |
| Qt 6.8.3 MinGW qmake | 通过，生成 `build/final-verify/Makefile`。 |
| 传输层反向依赖检查 | 通过，`src/core` 不再直接包含 `TaskManager.h`；状态码统一来自 `src/core/network/TransferTypes.h`。 |
| 传输共享契约 QtTest | 通过，`build/transfer-contract-test/release/transfer_contract_test.exe` 退出码 0。 |
| TaskManager 状态迁移与 fake Gateway QtTest | 通过，`build/task-manager-state-test/release/task_manager_state_test.exe` 报告 6 passed、0 failed。 |
| TaskTransferGateway 可替换端口构建 | 通过，`build/gateway-fake-verify/release/CloudBackupSystem.exe` 生成并启动，窗口标题为 `Cloud Backup`。 |
| 目录选择异步生命周期守卫构建 | 通过，`build/directory-lifetime-verify/release/CloudBackupSystem.exe` 生成并启动，窗口标题为 `Cloud Backup`。 |
| 当前架构边界 Release 构建 | 通过，`build/architecture-boundary-verify/release/CloudBackupSystem.exe` 完成 uic/moc/rcc、编译和链接。 |
| 当前架构边界启动冒烟 | 通过，启动 5 秒后进程保持响应；不以无边框窗口的 Win32 原生标题字段作为 UI 回归判据。 |
| TransferService 关闭守卫 QtTest | 通过，`build/transfer-service-shutdown/release/transfer_service_shutdown_test.exe` 退出码 0，6 个测试函数（4 个业务用例）0 failed。 |
| TransferService 队列调度 QtTest | 通过，`build/transfer-service-queue-dispatch/release/transfer_service_shutdown_test.exe` 新增多请求排空用例，两个排队请求均回传错误和失败状态。 |
| HomeDownloadSelectionPolicy QtTest | 通过，`build/home-download-selection-policy/release/home-download-selection-policy-test.exe` 退出码 0，3 个全选状态边界用例通过。 |
| DirectoryPathNavigation QtTest | 通过，`build/directory-path-navigation/release/directory_path_navigation_test.exe` 退出码 0，2 个父路径/面包屑边界用例通过。 |
| HomeTaskErrorPolicy QtTest | 通过，`build/home-task-error-policy/release/home-task-error-policy-test.exe` 退出码 0，3 个错误标题/去重键用例通过。 |
| TransferRequestPolicy QtTest | 通过，`build/transfer-request-policy/release/transfer_request_policy_test.exe` 退出码 0，3 个端点/恢复偏移用例通过。 |
| DirectoryService 慢网络生命周期 QtTest | 通过，`build/service-async-slow-network/release/service_async_slow_network_test.exe` 退出码 0，停止同步和服务析构两条迟到响应用例通过。 |
| 分层依赖自动闸门 | 通过，`powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify-layer-dependencies.ps1` 退出码 0。 |
| TaskPresentationPolicy QtTest | 通过，`build/task-presentation-policy/release/task-presentation-policy-test.exe` 退出码 0，4 个展示规则用例通过。 |
| DirectorySelectionPolicy QtTest | 通过，`build/directory-selection-policy/release/directory_selection_policy_test.exe` 退出码 0，2 个目录选择结果用例通过。 |
| DirectoryFileTypePolicy QtTest | 通过，`build/directory-file-type-policy/release/directory_file_type_policy_test.exe` 退出码 0；文本、图片、音频、视频、压缩、目录和可执行文件筛选/展示规则通过，包含 `/usr/bin/bash` 这类无后缀路径的保守识别。 |
| HomeTaskStatusPolicy QtTest | 通过，`build/home-task-status-policy/release/home-task-status-policy-test.exe` 退出码 0，3 个终态判定用例通过。 |
| QtTest 一键矩阵 | 通过，`tools/run-qt-test-matrix.ps1` 默认模式构建并运行 21 个 QtTest 工程，21 passed、0 failed；Linux/WSL native E2E 按设计未纳入本次默认运行。 |
| 四个热点实现文件职责拆分 | 通过，`HomeWidgeComposition.cpp`/`HomeWidgeRuntimeFeedback.cpp`、`DirectoryPageControllerDialogs.cpp`、`TransferServiceFileTransfer.cpp`、`TaskModuleScheduling.cpp` 已纳入 qmake；主程序 Release 链接和 5 秒启动冒烟通过，原公开接口保持不变。 |
| 三服务异步生命周期 QtTest | 通过，`build/service-async-lifetime-test/release/service_async_lifetime_test.exe` 退出码 0，5 个测试函数、0 failed；活动上传析构路径曾复现锁等待问题并已修复。 |
| Home 任务反馈摘要 QtTest | 通过，`build/home-task-feedback-summary/release/home-task-feedback-summary-test.exe` 退出码 0，2 个业务测试、0 failed。 |
| DirectoryLoadCoordinator QtTest | 通过，`build/directory-load-coordinator/release/directory_load_coordinator_test.exe` 退出码 0，3 个业务测试、0 failed。 |
| TransferService Qt socket fixture E2E | 通过，`build/transfer-service-e2e/release/transfer_service_e2e_test.exe` 退出码 0，上传/下载 round-trip、慢下载暂停/恢复/取消和协议字段断言通过；fixture 不替代 Linux 部署联调。 |
| HomeFileRowPresentation QtTest | 通过，`build/unified-home-file-row-presentation/release/home-file-row-presentation-test.exe` 退出码 0，2 个展示映射/大小边界用例通过。 |
| DirectoryTabPresentation QtTest | 通过，`build/unified-directory-tab-presentation/release/directory_tab_presentation_test.exe` 退出码 0，2 个页签标题/节点名规则用例通过。 |
| TransferControlState QtTest | 通过，`build/unified-transfer-control-state/release/transfer_control_state_test.exe` 退出码 0，3 个控制状态规则用例通过。 |
| Qt 客户端 -> Linux server.cpp E2E | 通过，`build/unified-transfer-service-linux-e2e/release/transfer_service_linux_e2e_test.exe` 退出码 0；TransferControlState 重构后真实 Linux 服务端上传和断点下载字节校验仍通过，不覆盖 UI/任务层控制。 |
| Linux 服务端协议 Smoke | 通过，WSL Ubuntu-D ELF 构建后完成 110843 字节上传/下载 SHA-256、目录列表、文件大小和 `../` 拒绝。 |
| Release 编译链接 | 通过，生成 `CloudBackupSystem.exe`。 |
| uic/moc/rcc | 通过，9 个 `.ui` 均被处理。 |
| 启动冒烟 | 通过，副本程序创建真实窗口并正常退出清理。 |
| 窗口截图 | 通过，截图保存于 `build/final-verify/ui-smoke.png`。 |
| 源码/UI 哈希回归 | 结构切片前通过；后续逻辑切片的变更已分别提交并记录。 |
| `.ui`/`resource.qrc` 哈希 | 通过，副本与原项目按文件名匹配的 9 个 `.ui` 文件和 `resource.qrc` 一致；源码目录迁移不等于 UI 内容变化。 |

## 编译警告记录

- 早期切片曾记录 `HomeWidge.cpp` 的未使用文本辅助函数；该代码已在后续纯辅助逻辑切片中移除。
- `TransferService.cpp` 的 MSVC 专用 `#pragma comment` 在 MinGW 下不参与链接；本轮协议边界抽取没有新增同类警告。

这些是历史构建记录，不作为当前功能已验证的替代证据。

## 尚未覆盖

- Qt 客户端 UI 直连真实 Linux 服务端的节点/目录、任务层暂停/恢复/取消、网络抖动和完整 UI 流程；独立 Qt 客户端联调已覆盖 TransferService native socket 的真实上传和断点下载，本地 fixture 另覆盖慢下载控制，但两者都不替代 UI 端到端证据。
- 多文件任务、服务端异常、长时间运行和节点重启后的实际 UI 行为。
- fake Gateway 驱动的深层依赖解耦行为回归。
