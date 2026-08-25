# Linux/WSL 服务端

`server.cpp` 是独立的 C++17 POSIX Socket 服务端，不依赖 Qt。它只处理受控存储根目录内的目录查询、上传、下载和偏移续传。

## 编译与启动

在 Linux 或 WSL 中，从仓库根目录执行：

```bash
mkdir -p build/server
g++ -std=c++17 -O2 -pthread server/server.cpp -o build/server/backup_server
mkdir -p /tmp/cloud-backup-demo-root
./build/server/backup_server /tmp/cloud-backup-demo-root
```

默认监听 `10000` 端口；如果端口被占用，程序会按实现尝试后续端口并在终端打印最终端口。第二个参数是服务端根目录，建议使用专门的临时演示目录，不要指向用户主目录。

## 协议演示

客户端使用自定义文本命令加二进制数据：

- `filelist|<path>`：列出根目录内的文件和目录；
- `fileput|<path>|<name>|<size>|<offset>`：上传并协商已有偏移；
- `filesave|<name>`：先查询远程文件大小；
- `filesave|<name>|<start>|<end>`：按范围读取文件内容。

协议 Smoke：

```bash
python3 tests/server_protocol_smoke.py ./build/server/backup_server
```

该脚本使用临时根目录验证上传、目录查询、下载字节摘要以及 `../` 路径拒绝。它不替代完整 Windows UI 到 Linux 服务端联调。

## 安全边界

当前服务端没有账号、认证、TLS、访问控制或多租户隔离，只能在本机、WSL 或明确受控网络中运行。路径规范化和参数校验用于演示协议边界，不等于生产级安全方案。
