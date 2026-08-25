/*
 * 目录网络服务实现：读取远程文件列表并维护按节点划分的自动同步定时器。
 * 目录命令复用 NodeService 的连接；目录列表读取使用请求级临时 socket。
 */
#include "DirectoryService.h"

#include "NodeService.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QMetaObject>
#include <QMutexLocker>
#include <QPointer>
#include <QTcpSocket>
#include <QThreadPool>
#include <QTimer>

DirectoryService::DirectoryService(NodeService *nodeService, QObject *parent)
    : QObject(parent)
    , m_nodeService(nodeService)
{
    // 目录命令复用 NodeService 的连接能力，列表读取仍按请求创建临时 socket。
}

DirectoryService::~DirectoryService()
{
    // 先使所有在途 worker 失效，再删除定时器；共享令牌的生命周期独立于 QObject。
    const QList<QSharedPointer<std::atomic_bool>> cancellationTokens = m_syncCancellation.values();
    for (const QSharedPointer<std::atomic_bool> &token : cancellationTokens) {
        if (token) {
            token->store(true, std::memory_order_release);
        }
    }

    const QList<QString> nodeIds = m_syncTimers.keys();
    for (const QString &nodeId : nodeIds) {
        stopFileListSync(nodeId);
    }
}

/**
 * @brief 读取指定节点当前路径下的远程目录列表。
 * @param nodeId 目标节点 ID。
 * @param path 目标目录路径。
 * @return 当前目录下的文件列表；读取失败时返回空列表。
 */
QList<NetworkFileInfo> DirectoryService::getFileInfoList(const QString &nodeId, const QString &path)
{
    if (!m_nodeService) {
        return {};
    }

    const NetworkNodeInfo node = m_nodeService->getNodeInfo(nodeId);
    if (node.nodeId.isEmpty()) {
        return {};
    }

    return fetchFileInfoList(node, path);
}

QList<NetworkFileInfo> DirectoryService::fetchFileInfoList(const NetworkNodeInfo &node,
                                                           const QString &path)
{
    // 该函数只接收值对象，允许线程池读取目录而不访问 DirectoryService 成员。
    QList<NetworkFileInfo> emptyResult;

    QTcpSocket socket;
    socket.connectToHost(node.ip, node.port);
    if (!socket.waitForConnected(10000)) {
        qWarning() << "[DirectoryService] 连接失败:" << node.ip << node.port;
        return emptyResult;
    }

    const QByteArray commandData = "filelist|" + path.toUtf8() + "\n";
    socket.write(commandData);
    if (!socket.waitForBytesWritten(3000)) {
        qWarning() << "[DirectoryService] 发送命令失败";
        return emptyResult;
    }

    if (!socket.waitForReadyRead(10000)) {
        return emptyResult;
    }

    QByteArray response = socket.readAll();
    while (!response.contains('\n') && socket.waitForReadyRead(500)) {
        response += socket.readAll();
    }

    QList<NetworkFileInfo> fileInfoList;
    if (response.isEmpty()) {
        return fileInfoList;
    }

    const QByteArray firstLine = response.contains('\n')
        ? response.left(response.indexOf('\n'))
        : response;
    const QStringList items = QString::fromUtf8(firstLine).split("|", Qt::SkipEmptyParts);
    for (int i = 0; i + 2 < items.size(); i += 3) {
        NetworkFileInfo info;
        info.fileName = items[i];
        info.filePath = (path == "/" || path.endsWith("/")) ? (path + items[i]) : (path + "/" + items[i]);
        info.fileSize = items[i + 1].toLongLong();
        info.isDirectory = (items[i + 2].toInt() == 1);
        info.modifyTime = QDateTime::currentDateTime();
        if (info.fileName == "..") {
            info.isDirectory = true;
        }
        fileInfoList.append(info);
    }

    return fileInfoList;
}

NetworkFileInfo DirectoryService::getFileInfo(const QString &nodeId, const QString &filePath)
{
    // 这是路径元数据辅助接口，不代表已经从服务端取得文件大小或修改时间。
    Q_UNUSED(nodeId);
    NetworkFileInfo info;
    info.filePath = filePath;
    info.fileName = QFileInfo(filePath).fileName();
    return info;
}

/**
 * @brief 为指定节点启动目录自动同步。
 * @param nodeId 目标节点 ID。
 * @param interval 同步间隔，内部会限制到当前允许的最小值。
 */
void DirectoryService::startFileListSync(const QString &nodeId, int interval)
{
    stopFileListSync(nodeId);

    const QSharedPointer<std::atomic_bool> cancellationToken =
        QSharedPointer<std::atomic_bool>::create(false);
    m_syncCancellation[nodeId] = cancellationToken;
    QPointer<DirectoryService> serviceGuard(this);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [serviceGuard, cancellationToken, nodeId]() {
        if (!serviceGuard || cancellationToken->load(std::memory_order_acquire)) {
            return;
        }

        DirectoryService *service = serviceGuard.data();
        if (!service || !service->m_nodeService) {
            return;
        }

        QString currentPath;
        QList<NetworkFileInfo> lastList;
        {
            QMutexLocker locker(&service->m_fileListMutex);
            currentPath = service->m_currentDirectories.value(nodeId, "./");
            lastList = service->m_lastFileLists.value(nodeId);
        }

        // 在服务线程复制节点信息；worker 只使用快照，避免跨线程读取 NodeService 成员。
        const NetworkNodeInfo node = service->m_nodeService->getNodeInfo(nodeId);
        if (node.nodeId.isEmpty()) {
            return;
        }

        // 自动同步只在目录层比较前后快照，页面层只接收真正有变化的结果。
        QThreadPool::globalInstance()->start([serviceGuard, cancellationToken, nodeId,
                                              node, currentPath, lastList]() {
            if (cancellationToken->load(std::memory_order_acquire)) {
                return;
            }

            const QList<NetworkFileInfo> currentList = fetchFileInfoList(node, currentPath);
            if (cancellationToken->load(std::memory_order_acquire) || !serviceGuard) {
                return;
            }

            bool changed = (currentList.size() != lastList.size());
            if (!changed) {
                for (int i = 0; i < currentList.size(); ++i) {
                    if (currentList[i].fileName != lastList[i].fileName ||
                        currentList[i].filePath != lastList[i].filePath ||
                        currentList[i].fileSize != lastList[i].fileSize ||
                        currentList[i].isDirectory != lastList[i].isDirectory) {
                        changed = true;
                        break;
                    }
                }
            }

            if (changed && serviceGuard && !cancellationToken->load(std::memory_order_acquire)) {
                QMetaObject::invokeMethod(serviceGuard.data(), [serviceGuard, cancellationToken,
                                                                 nodeId, currentList]() {
                    if (!serviceGuard || cancellationToken->load(std::memory_order_acquire)) {
                        return;
                    }
                    {
                        QMutexLocker locker(&serviceGuard->m_fileListMutex);
                        serviceGuard->m_lastFileLists[nodeId] = currentList;
                    }
                    emit serviceGuard->fileListUpdated(nodeId, currentList);
                }, Qt::QueuedConnection);
            }
        });
    });

    m_syncTimers[nodeId] = timer;
    const int syncInterval = qMax(interval, 10000);
    timer->start(syncInterval);
}

void DirectoryService::stopFileListSync(const QString &nodeId)
{
    if (const auto token = m_syncCancellation.take(nodeId)) {
        token->store(true, std::memory_order_release);
    }

    if (QTimer *timer = m_syncTimers.take(nodeId)) {
        timer->stop();
        delete timer;
    }
}

/**
 * @brief 执行远程目录命令。
 * @param cmd 目录命令类型。
 * @param name 目标目录或文件名。
 * @param nodeId 目标节点 ID。
 * @return 命令执行是否成功。
 */
bool DirectoryService::command(DirectoryCommand cmd, const QString &name, const QString &nodeId)
{
    if (!m_nodeService) {
        return false;
    }

    QTcpSocket *socket = m_nodeService->getConnection(nodeId);
    if (!socket) {
        return false;
    }

    QByteArray commandData;
    if (cmd == DirectoryCommand::Create) {
        commandData = "createdir|" + name.toUtf8() + "\n";
    } else {
        commandData = "deletefile|" + name.toUtf8() + "\n";
    }

    socket->write(commandData);
    if (!socket->waitForBytesWritten(3000)) {
        qWarning() << "[DirectoryService] 发送指令超时";
        return false;
    }

    if (socket->waitForReadyRead(10000)) {
        const QByteArray response = socket->readAll();
        return response.startsWith("OK");
    }

    return false;
}

bool DirectoryService::changeDirectory(const QString &nodeId, const QString &path)
{
    // 当前方法只保存下一次自动同步使用的路径，不主动触发网络请求。
    QMutexLocker locker(&m_fileListMutex);
    m_currentDirectories[nodeId] = path;
    return true;
}
