# Linux/WSL 服务端

`server.cpp` 是独立的 C++17 POSIX Socket 服务端，不依赖 Qt。它处理启动根目录内的目录查询、上传、下载和偏移续传，并支持全局只读浏览模式。

## 编译与启动

在 Linux 或 WSL 中，从仓库根目录执行：

```bash
mkdir -p build/server
g++ -std=c++17 -O2 -pthread server/server.cpp -o build/server/backup_server
mkdir -p /tmp/cloud-backup-demo-root
./build/server/backup_server --root /tmp/cloud-backup-demo-root
```

默认监听 `10000` 端口；如果端口被占用，程序会按实现尝试后续端口并在终端打印最终端口。第二个参数是服务端根目录，建议使用专门的临时演示目录，不要指向用户主目录。

如果要直接使用已有 Linux/WSL 目录，不需要复制文件：

```bash
chmod +x server/run-wsl-server.sh
./server/run-wsl-server.sh /mnt/d/Desktop/cloud-backup-demo
```

客户端里的虚拟 `/` 就是这里传入的目录。默认模式允许在根目录内读写；需要浏览 Linux 主机整棵目录树时，可以显式启动只读模式：

```bash
./server/run-wsl-server.sh / --read-only
```

只读模式允许 `filelist` 和 `filesave` 访问根目录下的目录和文件，但会拒绝上传、创建目录和删除操作。若要在全局浏览的同时保留上传演示，先创建专用写入目录，再显式指定它：

```bash
mkdir -p /tmp/cloud-backup-write-root
./server/run-wsl-server.sh / --write-root /tmp/cloud-backup-write-root
```

客户端仍显示统一的虚拟 `/`，面包屑和远程目录选择可以浏览整棵树；写操作只会落在 `--write-root` 及其子目录内。不要把可写根设为 `/`，也不要把没有认证和 TLS 的服务端暴露到公网。

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

当前服务端没有账号、认证、TLS、访问控制或多租户隔离，只能在本机、WSL 或明确受控网络中运行。只读模式适合全局浏览；可写模式必须使用专用 `--write-root`。路径规范化、符号链接边界和参数校验用于演示协议边界，不等于生产级安全方案。
