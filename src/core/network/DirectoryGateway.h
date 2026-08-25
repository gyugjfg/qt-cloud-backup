#ifndef DIRECTORYGATEWAY_H
#define DIRECTORYGATEWAY_H

#include "NetworkTypes.h"

#include <QObject>

class NetWork;

// 目录能力的窄适配器，负责目录读取和自动刷新，不暴露 NetWork 的节点/传输入口。
class DirectoryGateway final : public QObject
{
    Q_OBJECT

public:
    using FileInfo = NetworkFileInfo;

    // Gateway 只转发目录能力，并把异步更新信号重新发布给目录 Feature。
    explicit DirectoryGateway(NetWork *network, QObject *parent = nullptr);

    /** 读取指定节点和路径的远程文件快照。 */
    QList<FileInfo> fileInfoList(const QString &nodeId, const QString &path = QStringLiteral("./"));
    /** 查询单个远程文件或目录的元数据。 */
    FileInfo fileInfo(const QString &nodeId, const QString &filePath);
    /** 开启指定节点的周期性目录刷新。 */
    void startFileListSync(const QString &nodeId, int interval = 5000);
    /** 停止指定节点的周期性目录刷新。 */
    void stopFileListSync(const QString &nodeId);

signals:
    /** 目录快照发生变化时回传值对象，不在工作线程触碰 QWidget。 */
    void fileListUpdated(const QString &nodeId, const QList<FileInfo> &fileList);

private:
    NetWork *m_network;
};

#endif // DIRECTORYGATEWAY_H
