/* 目录异步协调实现：后台只抓取值对象，主线程才执行调用方回调。 */
#include "DirectoryLoadCoordinator.h"

#include <QMetaObject>
#include <QPointer>
#include <QThreadPool>

#include <utility>

DirectoryLoadCoordinator::DirectoryLoadCoordinator(Fetcher fetcher, QObject *parent)
    : QObject(parent)
    , m_fetcher(std::move(fetcher))
    , m_generation(QSharedPointer<std::atomic<quint64>>::create(0))
{
}

DirectoryLoadCoordinator::~DirectoryLoadCoordinator()
{
    cancelAll();
}

quint64 DirectoryLoadCoordinator::request(const QString &nodeId,
                                          const QString &path,
                                          ResultHandler handler)
{
    const quint64 requestId = m_generation->fetch_add(1, std::memory_order_acq_rel) + 1;
    const Generation generation = m_generation;
    const Fetcher fetcher = m_fetcher;
    const QPointer<DirectoryLoadCoordinator> coordinatorGuard(this);

    QThreadPool::globalInstance()->start([coordinatorGuard, generation, fetcher,
                                          requestId, nodeId, path,
                                          handler = std::move(handler)]() mutable {
        if (!coordinatorGuard || !fetcher
            || generation->load(std::memory_order_acquire) != requestId) {
            return;
        }

        const QList<NetworkFileInfo> fileList = fetcher(nodeId, path);
        if (generation->load(std::memory_order_acquire) != requestId) {
            return;
        }

        QMetaObject::invokeMethod(coordinatorGuard.data(),
                                  [coordinatorGuard, generation, requestId, nodeId, path,
                                   fileList, handler = std::move(handler)]() mutable {
            if (!coordinatorGuard || generation->load(std::memory_order_acquire) != requestId
                || !handler) {
                return;
            }
            handler(requestId, nodeId, path, fileList);
        }, Qt::QueuedConnection);
    });

    return requestId;
}

void DirectoryLoadCoordinator::cancelAll()
{
    m_generation->fetch_add(1, std::memory_order_acq_rel);
}
