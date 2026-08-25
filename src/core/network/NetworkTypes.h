#ifndef NETWORKTYPES_H
#define NETWORKTYPES_H

#include <QDateTime>
#include <QString>

// 网络层公共数据结构，避免节点 / 目录 / 传输各自维护重复类型。
struct NetworkNodeInfo
{
    QString nodeId;   ///< 节点唯一标识，持久化、目录请求和任务目标均使用此值。
    QString nodeName; ///< 面向用户展示的节点名称。
    QString ip;       ///< 节点服务端 IPv4 地址。
    int port = 0;     ///< 节点服务端 TCP 端口。
    int status = 0;   ///< 在线状态：0 离线、1 在线、2 忙碌。
};

struct NetworkFileInfo
{
    QString fileName;       ///< 文件或目录名称。
    QString filePath;       ///< 远程路径，和显示名称分开传递。
    qint64 fileSize = 0;    ///< 文件大小，目录通常为 0。
    QDateTime modifyTime;   ///< 服务端返回的最后修改时间。
    bool isDirectory = false; ///< true 表示目录，下载入口必须拒绝该项。
};

#endif // NETWORKTYPES_H
