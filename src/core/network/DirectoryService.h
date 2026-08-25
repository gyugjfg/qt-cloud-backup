#ifndef DIRECTORYSERVICE_H
#define DIRECTORYSERVICE_H

#include "NetworkTypes.h"

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QSharedPointer>

#include <atomic>

class NodeService;
class QTimer;

// 目录能力层：负责远程目录读取、当前路径维护和目录同步。
class DirectoryService : public QObject
{
    Q_OBJECT

public:
    enum class DirectoryCommand {
        Create,
        Delete
    };

    /** 创建目录服务；目录命令复用 NodeService 的连接管理。 */
    explicit DirectoryService(NodeService *nodeService, QObject *parent = nullptr);
    ~DirectoryService();

    /** 读取远程目录快照；返回值不暴露内部缓存。 */
    QList<NetworkFileInfo> getFileInfoList(const QString &nodeId, const QString &path = "./");
    /** 根据路径生成轻量文件元数据；当前不额外发起远程查询。 */
    NetworkFileInfo getFileInfo(const QString &nodeId, const QString &filePath);
    /** 启动指定节点的周期性目录同步。 */
    void startFileListSync(const QString &nodeId, int interval = 5000);
    /** 停止指定节点的周期性目录同步。 */
    void stopFileListSync(const QString &nodeId);
    /** 执行新建/删除目录命令，返回服务端是否成功。 */
    bool command(DirectoryCommand cmd, const QString &name, const QString &nodeId);
    /** 仅更新服务层记录的当前目录；实际读取仍由 getFileInfoList 发起。 */
    bool changeDirectory(const QString &nodeId, const QString &path);

signals:
    /** 自动同步检测到目录快照变化时发出。 */
    void fileListUpdated(const QString &nodeId, const QList<NetworkFileInfo> &fileList);

private:
    /** 在线程池中解析目录时只依赖值快照，避免 worker 触碰服务对象成员。 */
    static QList<NetworkFileInfo> fetchFileInfoList(const NetworkNodeInfo &node,
                                                    const QString &path);

    NodeService *m_nodeService;
    QMap<QString, QTimer*> m_syncTimers;
    QMap<QString, QSharedPointer<std::atomic_bool>> m_syncCancellation;
    QMap<QString, QList<NetworkFileInfo>> m_lastFileLists;
    QMap<QString, QString> m_currentDirectories;
    mutable QMutex m_fileListMutex;
};

#endif // DIRECTORYSERVICE_H
