#ifndef TRANSFERREQUESTPOLICY_H
#define TRANSFERREQUESTPOLICY_H

#include <QString>
#include <QtGlobal>

/**
 * @brief 传输请求进入执行器前的无状态规则。
 *
 * 该策略只处理可由请求字段和本地文件长度决定的值，不访问 socket、文件或
 * QObject。TransferService 仍负责信号回流和资源生命周期，避免规则判断反向
 * 依赖任务层或 UI。
 */
namespace TransferRequestPolicy {

/**
 * @brief 判断节点连接参数是否满足执行器的既有前置条件。
 * @param nodeId 节点唯一标识。
 * @param nodeIp 节点地址。
 * @param nodePort 节点端口。
 * @return 三项均有效时返回 true。
 */
inline bool hasUsableNodeEndpoint(const QString &nodeId,
                                  const QString &nodeIp,
                                  int nodePort)
{
    return !nodeId.isEmpty() && !nodeIp.isEmpty() && nodePort > 0;
}

/**
 * @brief 将上传恢复偏移限制在本地文件长度范围内。
 * @param requestedOffset 任务记录或调用方请求的偏移。
 * @param fileSize 本地文件长度。
 * @return 可安全用于 QFile::seek 的偏移。
 */
inline qint64 clampUploadOffset(qint64 requestedOffset, qint64 fileSize)
{
    return qBound<qint64>(0, requestedOffset, fileSize);
}

/**
 * @brief 根据目标文件实际长度解析下载恢复偏移。
 *
 * 当本地文件比服务端文件更长时，从零开始，避免在本地保留空洞或错误尾部；
 * 否则使用本地长度，但不超过服务端文件长度。
 */
inline qint64 resolveDownloadOffset(qint64 existingSize, qint64 fileSize)
{
    if (existingSize > fileSize) {
        return 0;
    }
    return qMin(existingSize, fileSize);
}

} // namespace TransferRequestPolicy

#endif // TRANSFERREQUESTPOLICY_H
