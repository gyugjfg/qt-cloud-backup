#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <algorithm>
#include <filesystem>
#include <cerrno>

using namespace std;
namespace fs = std::filesystem;

const int PORT = 10000;
const int BUFFER_SIZE = 1048576; // 1MB，与客户端一致
fs::path SERVER_ROOT;
fs::path WRITE_ROOT;
bool READ_ONLY_MODE = false;

// ==================== 工具函数 ====================

// 分割字符串函数
vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// 安全发送函数，检查返回值
// 返回 true 表示发送成功，false 表示连接已断开
bool safeSend(int sock, const char *data, int len) {
    int totalSent = 0;
    while (totalSent < len) {
        int bytesSent = send(sock, data + totalSent, len - totalSent, 0);
        if (bytesSent <= 0) {
            cerr << "send() 失败或连接断开: " << strerror(errno) << endl;
            return false;
        }
        totalSent += bytesSent;
    }
    return true;
}

bool safeSend(int sock, const string &data) {
    return safeSend(sock, data.c_str(), data.size());
}

// 安全接收函数，检查返回值
// 返回实际接收的字节数，<=0 表示连接断开或出错
int safeRecv(int sock, char *buf, int len) {
    int bytesRead = recv(sock, buf, len, 0);
    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            cerr << "recv() 连接已关闭" << endl;
        } else {
            cerr << "recv() 失败: " << strerror(errno) << endl;
        }
        return bytesRead;
    }
    return bytesRead;
}

bool safeRecvLine(int sock, string &line, size_t maxLength = 64 * 1024) {
    line.clear();
    char ch = 0;
    while (line.size() < maxLength) {
        const int bytesRead = recv(sock, &ch, 1, 0);
        if (bytesRead <= 0) {
            return false;
        }
        if (ch == '\n') {
            return true;
        }
        if (ch != '\r') {
            line.push_back(ch);
        }
    }
    return false;
}

bool isPathWithin(const fs::path &candidate, const fs::path &root) {
    auto candidateIt = candidate.begin();
    auto rootIt = root.begin();
    for (; rootIt != root.end(); ++rootIt, ++candidateIt) {
        if (candidateIt == candidate.end() || *candidateIt != *rootIt) {
            return false;
        }
    }
    return true;
}

bool canonicalizePath(const fs::path &input, fs::path &output) {
    std::error_code pathError;
    output = fs::weakly_canonical(input, pathError);
    return !pathError && output.is_absolute();
}

bool resolveServerPath(const string &requestedPath, fs::path &resolvedPath) {
    fs::path relativePath = requestedPath.empty() ? fs::path(".") : fs::path(requestedPath);
    // 客户端把 `/` 当作远程虚拟根目录；这里只去掉虚拟根前缀，
    // 仍然把最终路径限制在服务端启动工作目录内，避免把它当成本机绝对路径。
    if (relativePath.is_absolute()) {
        relativePath = relativePath.relative_path();
    }

    const fs::path candidate = (SERVER_ROOT / relativePath).lexically_normal();
    fs::path canonicalCandidate;
    if (!canonicalizePath(candidate, canonicalCandidate) ||
        !isPathWithin(canonicalCandidate, SERVER_ROOT)) {
        return false;
    }

    resolvedPath = canonicalCandidate;
    return true;
}

bool resolveWritableServerPath(const string &requestedPath, fs::path &resolvedPath) {
    if (READ_ONLY_MODE) {
        return false;
    }
    if (!resolveServerPath(requestedPath, resolvedPath) ||
        !isPathWithin(resolvedPath, WRITE_ROOT)) {
        return false;
    }
    return true;
}

bool parseNonNegativeLongLong(const string &text, long long &value) {
    try {
        size_t parsed = 0;
        value = stoll(text, &parsed);
        return parsed == text.size() && value >= 0;
    } catch (...) {
        return false;
    }
}

string joinServerPath(const string &basePath, const string &fileName) {
    fs::path base = basePath.empty() ? SERVER_ROOT : fs::path(basePath);
    fs::path target = (base / fileName).lexically_normal();
    return target.string();
}

// ==================== 命令处理函数 ====================

// 注意：以下函数均不负责关闭 clientSocket，由 handleClient 统一管理

// 处理 filelist 指令（获取文件列表）
void handleFileList(int clientSocket, const string &path) {
    DIR *dir;
    struct dirent *ent;
    string fileList;

    string dirPath = path;
    if (dirPath.empty()) {
        dirPath = ".";
    }
    if ((dir = opendir(dirPath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                fileList += ent->d_name;
                fileList += "|";

                struct stat statBuf;
                string fullPath;
                if (dirPath == ".") {
                    fullPath = ent->d_name;
                } else {
                    fullPath = dirPath + "/" + ent->d_name;
                }
                if (stat(fullPath.c_str(), &statBuf) == 0) {
                    fileList += to_string(statBuf.st_size);
                    fileList += "|";
                    if (S_ISDIR(statBuf.st_mode)) {
                        fileList += "1|";
                    } else {
                        fileList += "0|";
                    }
                } else {
                    fileList += "0|0|";
                }
            }
        }
        closedir(dir);
    } else {
        cerr << "打开目录失败: " << dirPath << endl;
        string response = "ERROR: 打开目录失败\n";
        safeSend(clientSocket, response);
        return;
    }

    safeSend(clientSocket, fileList);
}

// 安全创建目录（企业级安全版本）
bool safeCreateDirectory(const string& basePath, const string& fileName) {
    size_t lastSlash = fileName.find_last_of('/');
    if (lastSlash == string::npos) {
        return true; // 不需要创建目录
    }
    
    string directory = fileName.substr(0, lastSlash);
    fs::path fullPath = fs::weakly_canonical(basePath) / directory;
    fs::path allowedPath = fs::weakly_canonical(basePath);
    
    // 安全检查：确保路径在允许范围内，防止路径穿越攻击
    fs::path canonicalFullPath;
    if (!canonicalizePath(fullPath, canonicalFullPath) ||
        !isPathWithin(canonicalFullPath, allowedPath) ||
        !isPathWithin(canonicalFullPath, WRITE_ROOT)) {
        cerr << "安全警告: 非法路径穿越尝试: " << fileName << endl;
        return false;
    }
    
    // 输入验证：拒绝包含特殊字符的路径
    if (directory.find("..") != string::npos) {
        cerr << "安全警告: 路径包含非法字符: " << directory << endl;
        return false;
    }
    
    try {
        cout << "创建目录: " << fullPath << endl;
        fs::create_directories(fullPath);
        return true;
    } catch (const fs::filesystem_error& e) {
        cerr << "创建目录失败: " << e.what() << endl;
        return false;
    }
}

// 处理 fileput 指令（文件上传）
void handleFilePut(int clientSocket, const string &serverPath, const string &fileName, long long fileSize, long long startOffset) {
    cout << "开始处理文件上传: " << fileName << " (" << fileSize << " bytes)" << endl;

    if (!safeCreateDirectory(serverPath, fileName)) {
        string response = "ERROR: 非法路径\n";
        safeSend(clientSocket, response);
        return;
    }

    const string fullFilePath = joinServerPath(serverPath, fileName);
    long long existingSize = 0;
    struct stat existingStat;
    if (stat(fullFilePath.c_str(), &existingStat) == 0) {
        existingSize = existingStat.st_size;
    }

    // 服务端已落盘大小才是真实提交进度。客户端进度信号可能最多滞后一个发送周期，
    // 不能用较旧的 startOffset 把服务端偏移向后回退，否则恢复时会重复上传。
    const bool existingFileIsLonger = existingSize > fileSize;
    const long long serverOffset = existingFileIsLonger ? 0 : min(existingSize, fileSize);
    cout << "断点协商: clientOffset=" << startOffset
         << ", serverOffset=" << serverOffset << endl;

    ios::openmode mode = ios::binary | ios::out;
    if (serverOffset > 0) {
        mode |= ios::in;
    } else {
        mode |= ios::trunc;
    }

    fstream outFile(fullFilePath, mode);
    if (!outFile) {
        cerr << "创建文件失败: " << fullFilePath << endl;
        string response = "ERROR: 创建文件失败\n";
        safeSend(clientSocket, response);
        return;
    }

    if (serverOffset > 0) {
        outFile.seekp(serverOffset, ios::beg);
        if (!outFile) {
            cerr << "文件定位失败: " << fullFilePath << endl;
            string response = "ERROR: 文件定位失败\n";
            safeSend(clientSocket, response);
            return;
        }
    }

    string ack = "READY:" + to_string(serverOffset) + "\n";
    if (!safeSend(clientSocket, ack)) {
        outFile.close();
        return;
    }
    cout << "发送READY确认消息, serverOffset=" << serverOffset << endl;

    long long totalReceived = serverOffset;
    char *buffer = new char[BUFFER_SIZE];
    int progress = fileSize > 0 ? static_cast<int>((totalReceived * 100) / fileSize) : 100;

    while (totalReceived < fileSize) {
        int bytesToRead = min((long long)BUFFER_SIZE, fileSize - totalReceived);
        int bytesRead = recv(clientSocket, buffer, bytesToRead, 0);

        if (bytesRead > 0) {
            outFile.write(buffer, bytesRead);
            outFile.flush();
            if (!outFile) {
                cerr << "写入文件失败: " << fullFilePath << endl;
                outFile.close();
                string response = "ERROR: 写入文件失败\n";
                safeSend(clientSocket, response);
                delete[] buffer;
                return;
            }

            struct stat statBuf;
            if (stat(fullFilePath.c_str(), &statBuf) == 0) {
                totalReceived = statBuf.st_size;
            } else {
                totalReceived += bytesRead;
            }

            int newProgress = (int)((totalReceived * 100) / fileSize);
            if (newProgress != progress) {
                progress = newProgress;
                cout << "上传进度: " << progress << "% (" << totalReceived << "/" << fileSize << " bytes)" << endl;
            }
        } else if (bytesRead < 0) {
            cerr << "接收文件数据失败: " << strerror(errno) << endl;
            outFile.close();
            string response = "ERROR: 接收文件失败\n";
            safeSend(clientSocket, response);
            delete[] buffer;
            return;
        } else {
            cerr << "客户端连接关闭，接收数据中断" << endl;
            break;
        }
    }

    outFile.flush();
    outFile.close();
    delete[] buffer;
    struct stat finalStat;
    if (stat(fullFilePath.c_str(), &finalStat) == 0) {
        totalReceived = finalStat.st_size;
    }
    cout << "文件写入完成，接收了 " << totalReceived << " 字节" << endl;

    if (totalReceived >= fileSize) {
        string response = "OK:" + to_string(totalReceived) + "\n";
        safeSend(clientSocket, response);
        cout << "文件上传成功: " << fileName << endl;
    } else {
        string response = "ERROR: 文件不完整\n";
        safeSend(clientSocket, response);
        cout << "文件上传未完成，保留已接收部分: " << totalReceived << " 字节" << endl;
    }
}

// 处理 filesave 指令（文件下载）
// sendOnlySize=true 时只返回文件大小，不发送文件数据
void handleFileSave(int clientSocket, const string &filePath, long long startPos, long long endPos, bool sendOnlySize) {
    // 先获取文件大小（使用stat，与文件列表保持一致）
    struct stat statBuf;
    if (stat(filePath.c_str(), &statBuf) != 0) {
        cerr << "文件不存在: " << filePath << endl;
        string response = "ERROR: 文件不存在\n";
        safeSend(clientSocket, response);
        return;
    }
    long long fileSize = statBuf.st_size;

    // 检查文件是否存在并打开
    ifstream inFile(filePath, ios::binary);
    if (!inFile) {
        cerr << "文件打开失败: " << filePath << endl;
        string response = "ERROR: 文件打开失败\n";
        safeSend(clientSocket, response);
        return;
    }

    // 发送文件大小
    string sizeStr = to_string(fileSize) + "\n";
    if (!safeSend(clientSocket, sizeStr)) {
        inFile.close();
        return;
    }

    // 如果只是查询文件大小，到此结束
    if (sendOnlySize) {
        inFile.close();
        return;
    }

    // 调整结束位置
    if (endPos >= fileSize) {
        endPos = fileSize - 1;
    }

    // 确保起始位置不超过结束位置
    if (startPos > endPos) {
        cerr << "起始位置大于结束位置" << endl;
        string response = "ERROR: 起始位置大于结束位置\n";
        safeSend(clientSocket, response);
        inFile.close();
        return;
    }

    // 等待客户端确认
    char *buffer = new char[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    string confirmation;
    if (!safeRecvLine(clientSocket, confirmation) || confirmation != "OK") {
        cerr << "接收客户端确认失败" << endl;
        inFile.close();
        delete[] buffer;
        return;
    }

    // 定位到起始位置
    inFile.seekg(startPos, ios::beg);

    // 发送文件数据
    long long totalSent = 0;
    long long dataSize = endPos - startPos + 1;
    while (totalSent < dataSize) {
        int bytesToRead = min((long long)BUFFER_SIZE, dataSize - totalSent);
        inFile.read(buffer, bytesToRead);
        streamsize readCount = inFile.gcount();

        if (readCount == 0) {
            break;
        }

        // 确保所有数据都被发送
        char *ptr = buffer;
        streamsize remaining = readCount;
        while (remaining > 0) {
            int bytesSent = send(clientSocket, ptr, remaining, 0);
            if (bytesSent <= 0) {
                cerr << "发送文件数据失败" << endl;
                inFile.close();
                delete[] buffer;
                return;
            }
            remaining -= bytesSent;
            ptr += bytesSent;
            totalSent += bytesSent;
        }
    }

    inFile.close();
    delete[] buffer;
}

// 处理 createdir 指令（创建目录）
void handleCreateDir(int clientSocket, const string &dirName) {
    if (mkdir(dirName.c_str(), 0777) == 0) {
        string response = "OK\n";
        safeSend(clientSocket, response);
    } else {
        cerr << "创建目录失败: " << dirName << endl;
        string response = "ERROR: 创建目录失败\n";
        safeSend(clientSocket, response);
    }
}

// 处理 deletefile 指令（删除文件）
void handleDeleteFile(int clientSocket, const string &fileName) {
    if (remove(fileName.c_str()) == 0) {
        string response = "OK\n";
        safeSend(clientSocket, response);
    } else {
        cerr << "删除文件失败: " << fileName << endl;
        string response = "ERROR: 删除文件失败\n";
        safeSend(clientSocket, response);
    }
}

// ==================== 客户端连接处理 ====================

void handleClient(int clientSocket) {
    char *buffer = new char[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    string command;
    if (!safeRecvLine(clientSocket, command)) {
        delete[] buffer;
        close(clientSocket);
        return;
    }
    delete[] buffer;

    vector<string> parts = split(command, '|');

    if (parts.empty()) {
        cerr << "命令格式错误" << endl;
        string response = "ERROR: 命令格式错误\n";
        safeSend(clientSocket, response);
        close(clientSocket);
        return;
    }

    string cmd = parts[0];

    if (cmd == "filelist") {
        // filelist|path
        string path = parts.size() > 1 ? parts[1] : "";
        fs::path resolvedPath;
        if (!resolveServerPath(path, resolvedPath)) {
            string response = "ERROR: 路径不安全\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        handleFileList(clientSocket, resolvedPath.string());
    } else if (cmd == "fileput") {
        if (READ_ONLY_MODE) {
            safeSend(clientSocket, "ERROR: 服务端处于只读浏览模式\n");
            close(clientSocket);
            return;
        }
        // fileput|serverPath|fileName|fileSize|startOffset
        if (parts.size() != 5) {
            cerr << "fileput命令格式错误" << endl;
            string response = "ERROR: 命令格式错误\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        string serverPath = parts[1];
        string fileName = parts[2];
        fs::path resolvedServerPath;
        fs::path resolvedFilePath;
        if (!resolveWritableServerPath(serverPath, resolvedServerPath) ||
            !resolveWritableServerPath((fs::path(serverPath) / fileName).string(), resolvedFilePath)) {
            string response = "ERROR: 路径不安全\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        long long fileSize = 0;
        long long startOffset = 0;
        if (!parseNonNegativeLongLong(parts[3], fileSize) ||
            !parseNonNegativeLongLong(parts[4], startOffset)) {
            string response = "ERROR: 数字参数非法\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        cout << "接收到文件上传请求: " << fileName << " (" << fileSize << " bytes)" << endl;
        handleFilePut(clientSocket, resolvedServerPath.string(), fileName, fileSize, startOffset);
    } else if (cmd == "filesave") {
        // filesave|filePath （2个参数：只返回文件大小）
        // filesave|filePath|startPos|endPos （4个参数：发送文件数据）
        if (parts.size() != 2 && parts.size() != 4) {
            cerr << "filesave命令格式错误" << endl;
            string response = "ERROR: 命令格式错误\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        string filePath = parts[1];
        fs::path resolvedFilePath;
        if (!resolveServerPath(filePath, resolvedFilePath)) {
            string response = "ERROR: 路径不安全\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        if (parts.size() == 2) {
            // 只返回文件大小，不发送文件数据
            handleFileSave(clientSocket, resolvedFilePath.string(), 0, 0, true);
        } else {
            // 发送文件数据
            long long startPos = 0;
            long long endPos = 0;
            if (!parseNonNegativeLongLong(parts[2], startPos) ||
                !parseNonNegativeLongLong(parts[3], endPos)) {
                string response = "ERROR: 数字参数非法\n";
                safeSend(clientSocket, response);
                close(clientSocket);
                return;
            }
            handleFileSave(clientSocket, resolvedFilePath.string(), startPos, endPos, false);
        }
    } else if (cmd == "createdir") {
        if (READ_ONLY_MODE) {
            safeSend(clientSocket, "ERROR: 服务端处于只读浏览模式\n");
            close(clientSocket);
            return;
        }
        // createdir|dirName
        if (parts.size() != 2) {
            cerr << "createdir命令格式错误" << endl;
            string response = "ERROR: 命令格式错误\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        string dirName = parts[1];
        fs::path resolvedDirPath;
        if (!resolveWritableServerPath(dirName, resolvedDirPath)) {
            string response = "ERROR: 路径不安全\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        handleCreateDir(clientSocket, resolvedDirPath.string());
    } else if (cmd == "deletefile") {
        if (READ_ONLY_MODE) {
            safeSend(clientSocket, "ERROR: 服务端处于只读浏览模式\n");
            close(clientSocket);
            return;
        }
        // deletefile|fileName
        if (parts.size() != 2) {
            cerr << "deletefile命令格式错误" << endl;
            string response = "ERROR: 命令格式错误\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        string fileName = parts[1];
        fs::path resolvedDeletePath;
        if (!resolveWritableServerPath(fileName, resolvedDeletePath)) {
            string response = "ERROR: 路径不安全\n";
            safeSend(clientSocket, response);
            close(clientSocket);
            return;
        }
        handleDeleteFile(clientSocket, resolvedDeletePath.string());
    } else {
        cerr << "不支持的命令: " << cmd << endl;
        string response = "ERROR: 不支持的命令\n";
        safeSend(clientSocket, response);
    }

    // 统一关闭连接
    close(clientSocket);
}

// ==================== 主函数 ====================

int main(int argc, char *argv[]) {
    // 必须忽略 SIGPIPE 信号，防止写入已断开的连接时进程被杀死
    signal(SIGPIPE, SIG_IGN);

    fs::path requestedRoot = fs::current_path();
    fs::path requestedWriteRoot;
    bool writeRootProvided = false;
    bool positionalRootProvided = false;
    for (int index = 1; index < argc; ++index) {
        const string argument = argv[index];
        if (argument == "--read-only") {
            READ_ONLY_MODE = true;
        } else if (argument == "--root" && index + 1 < argc) {
            requestedRoot = argv[++index];
        } else if (argument == "--write-root" && index + 1 < argc) {
            requestedWriteRoot = argv[++index];
            writeRootProvided = true;
        } else if (argument.rfind("--", 0) == 0) {
            cerr << "未知参数: " << argument << endl;
            cerr << "用法: backup_server [root] [--root <path>] [--read-only] [--write-root <path>]" << endl;
            return 1;
        } else if (!positionalRootProvided) {
            // 保留旧版的第一个位置参数：backup_server <root>
            requestedRoot = argument;
            positionalRootProvided = true;
        } else {
            cerr << "多余的位置参数: " << argument << endl;
            return 1;
        }
    }
    std::error_code rootError;
    SERVER_ROOT = fs::weakly_canonical(requestedRoot, rootError);
    if (rootError || !fs::exists(SERVER_ROOT) || !fs::is_directory(SERVER_ROOT)) {
        cerr << "服务端根目录无效: " << requestedRoot << endl;
        return 1;
    }
    if (READ_ONLY_MODE && writeRootProvided) {
        cerr << "只读模式不能同时指定可写目录" << endl;
        return 1;
    }
    if (writeRootProvided) {
        std::error_code writeRootError;
        WRITE_ROOT = fs::weakly_canonical(requestedWriteRoot, writeRootError);
        if (writeRootError || !fs::exists(WRITE_ROOT) || !fs::is_directory(WRITE_ROOT) ||
            !isPathWithin(WRITE_ROOT, SERVER_ROOT)) {
            cerr << "可写目录无效或不在服务端根目录内: " << requestedWriteRoot << endl;
            return 1;
        }
    } else {
        WRITE_ROOT = SERVER_ROOT;
    }
    cout << "服务端文件根目录: " << SERVER_ROOT << endl;
    cout << "服务端模式: " << (READ_ONLY_MODE ? "只读浏览/下载" : "可写") << endl;
    cout << "服务端可写目录: " << (READ_ONLY_MODE ? "(无)" : WRITE_ROOT.string()) << endl;

    int serverSocket = -1;
    int port = PORT;
    struct sockaddr_in serverAddr;

    // 尝试找到一个可用的端口
    for (int i = 0; i < 10; i++) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            cerr << "创建socket失败: " << strerror(errno) << endl;
            return 1;
        }

        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            cerr << "设置socket选项失败: " << strerror(errno) << endl;
            close(serverSocket);
            serverSocket = -1;
            port++;
            continue;
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "绑定地址失败: " << strerror(errno) << "，尝试端口: " << port << endl;
            close(serverSocket);
            serverSocket = -1;
            port++;
            continue;
        }

        break;
    }

    if (serverSocket == -1) {
        cerr << "尝试10个端口后仍然失败，退出程序" << endl;
        return 1;
    }

    if (listen(serverSocket, 50) < 0) {
        cerr << "监听失败: " << strerror(errno) << endl;
        close(serverSocket);
        return 1;
    }

    cout << "服务器启动成功，监听端口: " << port << endl;

    // 接受客户端连接，每个连接创建一个线程
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (clientSocket < 0) {
            cerr << "接受连接失败: " << strerror(errno) << endl;
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        cout << "客户端连接: " << clientIP << endl;

        // 每个客户端连接创建一个独立线程
        thread(handleClient, clientSocket).detach();
    }

    close(serverSocket);
    return 0;
}
