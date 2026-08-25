#ifndef TRANSFERPROTOCOLCLIENT_H
#define TRANSFERPROTOCOLCLIENT_H

#include <QByteArray>
#include <QString>

/**
 * @brief 文件传输协议的 socket 边界。
 *
 * 本类只处理节点连接、完整发送、协议行读取和握手响应解析；
 * 文件读写、任务控制、进度信号与队列调度仍由 TransferService 负责。
 * socket 句柄仍以 int 形式传递，保持现有控制表和 Windows/Linux 行为不变。
 */
class TransferProtocolClient final
{
public:
    /** 连接节点并设置连接/读写超时，失败返回 -1。 */
    static int connectToNode(const QString &ip, int port, int timeoutSec = 10);

    /** 处理短写，直到整段协议或文件数据发送完成。 */
    static bool sendAll(int socketHandle, const char *data, qsizetype length);
    /** 读取一行协议响应，不吞掉换行之后的文件数据。 */
    static QByteArray receiveLine(int socketHandle);
    /** 接收一段文件数据，返回底层 recv 的字节数。 */
    static int receive(int socketHandle, char *buffer, int length);
    /** 关闭句柄；重复关闭由调用方生命周期保证。 */
    static void closeSocket(int socketHandle);

    /** 清理文件名首部，避免把协议分隔符带入命令。 */
    static QString sanitizedFileName(const QString &fileName);
    /** 生成 fileput 上传握手。 */
    static QByteArray uploadCommand(const QString &fileName, qint64 fileSize, qint64 startOffset);
    /** 生成 filesave 文件大小查询命令。 */
    static QByteArray downloadSizeCommand(const QString &fileName);
    /** 生成 filesave 区间下载命令。 */
    static QByteArray downloadRangeCommand(const QString &fileName, qint64 startOffset, qint64 endOffset);
    /** 解析 READY:<offset>。 */
    static bool parseReadyOffset(const QByteArray &response, qint64 &serverOffset);
    /** 解析 OK:<offset>。 */
    static bool parseOkOffset(const QByteArray &response, qint64 &serverOffset);
};

#endif // TRANSFERPROTOCOLCLIENT_H
