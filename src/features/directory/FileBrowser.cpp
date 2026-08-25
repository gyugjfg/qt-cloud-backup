#include "FileBrowser.h"
#include "DirectoryGateway.h"
#include "NodeGateway.h"
#include "FileTypePolicy.h"
#include <QAbstractItemView>
#include <QHeaderView>
#include <QThreadPool>
#include <QTreeWidgetItem>
#include <QColor>
#include <QMetaObject>

namespace {
QString displayFileType(const QString &fileName,
                        bool isDirectory,
                        const QString &filePath = QString())
{
    return FileTypePolicy::displayType(fileName, isDirectory, filePath);
}
}

FileBrowser::FileBrowser(DirectoryGateway *directoryGateway,
                         NodeGateway *nodeGateway,
                         QObject *parent)
    : QObject(parent)
    , m_directoryGateway(directoryGateway)
    , m_nodeGateway(nodeGateway)
    , m_requestCounter(0)
{
    // 文件浏览器统一吃目录更新信号，页面层不直接绑定底层目录回流。
    if (m_directoryGateway) {
        connect(m_directoryGateway, &DirectoryGateway::fileListUpdated,
                this, &FileBrowser::handleFileListUpdated);
    }
}

FileBrowser::~FileBrowser()
{
}

/**
 * @brief 配置目录页和下载页使用的文件树基础列布局。
 * @param tree 目标树控件。
 */
void FileBrowser::configureFileTree(QTreeWidget *tree) const
{
    if (!tree) {
        return;
    }

    tree->setColumnCount(4);
    tree->setHeaderLabels({
        QStringLiteral("文件名"),
        QStringLiteral("大小"),
        QStringLiteral("修改时间"),
        QStringLiteral("类型")
    });
    tree->setWordWrap(false);
    tree->setTextElideMode(Qt::ElideRight);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 100);
    tree->setColumnWidth(2, 180);
    tree->setColumnWidth(3, 80);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    if (QHeaderView *header = tree->header()) {
        header->setSectionResizeMode(0, QHeaderView::Interactive);
        header->setSectionResizeMode(1, QHeaderView::Fixed);
        header->setSectionResizeMode(2, QHeaderView::Interactive);
        header->setSectionResizeMode(3, QHeaderView::Fixed);
    }
}

/**
 * @brief 配置目录选择弹窗使用的目录树基础列布局。
 * @param tree 目标树控件。
 */
void FileBrowser::configureDirectorySelectionTree(QTreeWidget *tree) const
{
    if (!tree) {
        return;
    }

    tree->setColumnCount(3);
    tree->setHeaderLabels({QStringLiteral("目录名"), QStringLiteral("大小"), QStringLiteral("修改时间")});
    tree->setColumnWidth(0, 400);
    tree->setColumnWidth(1, 100);
    tree->setColumnWidth(2, 150);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
}

/**
 * @brief 用目录结果填充目录选择树。
 * @param tree 目标树控件。
 * @param fileList 当前目录结果。
 * @param parentItem 为空时填充根层；非空时填充某个展开目录的子项。
 */
void FileBrowser::populateDirectorySelectionTree(QTreeWidget *tree,
                                               const QList<NetworkFileInfo> &fileList,
                                                 QTreeWidgetItem *parentItem) const
{
    if (!tree) {
        return;
    }

    auto appendDirectoryItem = [](QTreeWidgetItem *targetItem, const NetworkFileInfo &fileInfo) {
        targetItem->setText(0, fileInfo.fileName);
        targetItem->setText(1, FileBrowser::formatFileSize(fileInfo.fileSize));
        targetItem->setText(2, fileInfo.modifyTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        targetItem->setData(0, Qt::UserRole, fileInfo.filePath);

        if (fileInfo.fileName != QStringLiteral("..")) {
            QTreeWidgetItem *placeholder = new QTreeWidgetItem(targetItem);
            placeholder->setText(0, QStringLiteral("正在加载..."));
            placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
            placeholder->setForeground(0, QColor(100, 180, 255));
        }
    };

    if (!parentItem) {
        tree->clear();
        QTreeWidgetItem *rootItem = new QTreeWidgetItem(tree);
        rootItem->setText(0, QStringLiteral("/"));
        rootItem->setData(0, Qt::UserRole, QStringLiteral("/"));

        int directoryCount = 0;
        for (const NetworkFileInfo &fileInfo : fileList) {
            if (!fileInfo.isDirectory) {
                continue;
            }

            QTreeWidgetItem *item = new QTreeWidgetItem(tree);
            appendDirectoryItem(item, fileInfo);
            ++directoryCount;
        }

        if (directoryCount == 0) {
            QTreeWidgetItem *item = new QTreeWidgetItem(tree);
            item->setText(0, QStringLiteral("暂无其他目录"));
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
            item->setForeground(0, QColor(150, 150, 150));
        }
        return;
    }

    while (parentItem->childCount() > 0) {
        delete parentItem->takeChild(0);
    }

    for (const NetworkFileInfo &fileInfo : fileList) {
        if (!fileInfo.isDirectory || fileInfo.fileName == QStringLiteral("..")) {
            continue;
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(parentItem);
        appendDirectoryItem(item, fileInfo);
    }
}

/**
 * @brief 用目录结果填充文件列表树。
 * @param tree 目标树控件。
 * @param fileList 当前目录结果。
 */
void FileBrowser::populateDirectoryListingTree(QTreeWidget *tree,
                                               const QList<NetworkFileInfo> &fileList) const
{
    if (!tree) {
        return;
    }

    tree->clear();
    for (const NetworkFileInfo &fileInfo : fileList) {
        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setCheckState(0, Qt::Unchecked);
        item->setText(0, fileInfo.fileName);
        item->setText(1, formatFileSize(fileInfo.fileSize));
        item->setText(2, fileInfo.modifyTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        item->setText(3, displayFileType(fileInfo.fileName,
                                         fileInfo.isDirectory,
                                         fileInfo.filePath));
        item->setData(0, Qt::UserRole, fileInfo.filePath);
        item->setData(1, Qt::UserRole, fileInfo.isDirectory);
        tree->addTopLevelItem(item);
    }
}

void FileBrowser::loadFileList(const QString &nodeId, const QString &path, QTreeWidget *tree)
{
    if (!tree || !m_directoryGateway) return;

    // 同一个树控件只认最后一次加载请求，避免切路径后旧结果回刷。
    quint64 requestId = ++m_requestCounter;
    m_pendingRequests[tree] = requestId;

    tree->clear();
    QTreeWidgetItem *loadingItem = new QTreeWidgetItem(tree);
    loadingItem->setText(0, "加载中...");
    loadingItem->setText(1, "");
    loadingItem->setText(2, "");
    loadingItem->setText(3, "");
    loadingItem->setFlags(loadingItem->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
    loadingItem->setForeground(0, QColor(100, 180, 255));
    tree->addTopLevelItem(loadingItem);

    QThreadPool::globalInstance()->start([this, nodeId, path, tree, requestId]() {
        QList<NetworkFileInfo> fileList = m_directoryGateway->fileInfoList(nodeId, path);

        QMetaObject::invokeMethod(this, [this, nodeId, path, tree, requestId, fileList]() {
            auto it = m_pendingRequests.find(tree);
            if (it == m_pendingRequests.end() || it.value() != requestId) {
                return;
            }

            if (!tree) return;

            m_pendingRequests.remove(tree);

            fillTreeWidget(tree, fileList, nodeId);
            emit fileListLoaded(nodeId, path);
        }, Qt::QueuedConnection);
    });
}

void FileBrowser::fillTreeWidget(QTreeWidget *tree,
                                 const QList<NetworkFileInfo> &fileList,
                                 const QString &nodeId)
{
    if (!tree) return;
    tree->clear();

    // 空列表时额外检查一次节点在线状态，避免页面只显示“暂无文件”。
    if (fileList.isEmpty()) {
        if (m_nodeGateway) {
            const bool nodeOnline = m_nodeGateway->checkNodeStatus(nodeId);
            if (!nodeOnline) {
                QTreeWidgetItem *item = new QTreeWidgetItem(tree);
                item->setText(0, "节点离线，请检查网络连接");
                item->setText(1, "");
                item->setText(2, "");
                item->setText(3, "");
                item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
                item->setForeground(0, QColor(255, 100, 100));
            } else {
                QTreeWidgetItem *item = new QTreeWidgetItem(tree);
                item->setText(0, "暂无文件");
                item->setText(1, "");
                item->setText(2, "");
                item->setText(3, "");
                item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
                item->setForeground(0, QColor(150, 150, 150));
            }
        }
        return;
    }

    for (const NetworkFileInfo &fileInfo : fileList) {
        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setCheckState(0, Qt::Unchecked);

        QString fileType = displayFileType(fileInfo.fileName,
                                           fileInfo.isDirectory,
                                           fileInfo.filePath);
        if (fileInfo.isDirectory) {
            item->setText(0, "📁 " + fileInfo.fileName);
        } else {
            QString extension = fileInfo.fileName.section(".", -1).toLower();
            if (extension == "txt") {
                item->setText(0, "📄 " + fileInfo.fileName);
            } else if (extension == "pdf") {
                item->setText(0, "📕 " + fileInfo.fileName);
            } else if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "gif") {
                item->setText(0, "🖼️ " + fileInfo.fileName);
            } else if (extension == "mp3" || extension == "wav" || extension == "flac") {
                item->setText(0, "🎵 " + fileInfo.fileName);
            } else if (extension == "mp4" || extension == "avi" || extension == "mkv") {
                item->setText(0, "🎬 " + fileInfo.fileName);
            } else if (extension == "zip" || extension == "rar" || extension == "7z") {
                item->setText(0, "📦 " + fileInfo.fileName);
            } else if (FileTypePolicy::isExecutableFile(fileInfo.fileName, fileInfo.filePath)) {
                item->setText(0, "💻 " + fileInfo.fileName);
            } else {
                item->setText(0, "📄 " + fileInfo.fileName);
            }
        }

        item->setText(1, formatFileSize(fileInfo.fileSize));
        item->setText(2, fileInfo.modifyTime.toString("yyyy-MM-dd HH:mm:ss"));
        item->setText(3, fileType);

        item->setData(0, Qt::UserRole, fileInfo.filePath);
        item->setData(1, Qt::UserRole, fileInfo.isDirectory);
        tree->addTopLevelItem(item);
    }
}
QString FileBrowser::getCurrentPath(const QString &nodeId) const
{
    return m_currentPaths.value(nodeId, "/");
}

void FileBrowser::setCurrentPath(const QString &nodeId, const QString &path)
{
    m_currentPaths[nodeId] = path;
}

QString FileBrowser::formatFileSize(qint64 size)
{
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 2);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}

/**
 * @brief 处理网络层目录更新信号，并向页面层转发统一加载完成通知。
 * @param nodeId 发生更新的节点 ID。
 * @param fileList 当前目录结果。
 */
void FileBrowser::handleFileListUpdated(const QString &nodeId, const QList<NetworkFileInfo> &fileList)
{
    Q_UNUSED(fileList);
    emit fileListLoaded(nodeId, "");
}
