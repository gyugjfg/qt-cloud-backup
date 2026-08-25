#ifndef DIRECTORYLOADCOORDINATOR_H
#define DIRECTORYLOADCOORDINATOR_H

#include "NetworkTypes.h"

#include <QObject>
#include <QSharedPointer>

#include <atomic>
#include <functional>

/**
 * @brief 目录读取的异步协调边界。
 *
 * 该对象只负责请求代际、后台抓取和主线程回调，不持有 QWidget，也不负责
 * 目录树展示。每次 request() 都会让前一代结果失效；销毁或 cancelAll()
 * 后，已在 QThreadPool 中运行的抓取最多完成自身 I/O，不再回写调用方。
 */
class DirectoryLoadCoordinator final : public QObject
{
public:
    using Fetcher = std::function<QList<NetworkFileInfo>(const QString &nodeId,
                                                          const QString &path)>;
    using ResultHandler = std::function<void(quint64 requestId,
                                             const QString &nodeId,
                                             const QString &path,
                                             const QList<NetworkFileInfo> &fileList)>;

    explicit DirectoryLoadCoordinator(Fetcher fetcher, QObject *parent = nullptr);
    ~DirectoryLoadCoordinator() override;

    /**
     * @brief 提交一次目录读取；新请求会使此前请求的结果失效。
     * @return 本次请求的单调递增代号。
     */
    quint64 request(const QString &nodeId, const QString &path, ResultHandler handler);

    /** 使当前及尚未回调的请求结果全部失效。 */
    void cancelAll();

private:
    using Generation = QSharedPointer<std::atomic<quint64>>;

    Fetcher m_fetcher;
    Generation m_generation;
};

#endif // DIRECTORYLOADCOORDINATOR_H
