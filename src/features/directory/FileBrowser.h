#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include "NetworkTypes.h"

#include <QObject>
#include <QMap>
#include <QString>
#include <QTreeWidget>

// 文件浏览能力层：负责目录请求下发、树控件填充和节点路径状态维护。
class DirectoryGateway;
class NodeGateway;

class FileBrowser : public QObject
{
    Q_OBJECT

public:
    explicit FileBrowser(DirectoryGateway *directoryGateway,
                         NodeGateway *nodeGateway,
                         QObject *parent = nullptr);
    ~FileBrowser();

    /**
     * @brief 异步加载指定节点下的目录内容并回填到树控件。
     * @param nodeId 目标节点 ID。
     * @param path 目标目录路径。
     * @param tree 需要回填的树控件。
     */
    void loadFileList(const QString &nodeId, const QString &path, QTreeWidget *tree);
    void configureFileTree(QTreeWidget *tree) const;
    void configureDirectorySelectionTree(QTreeWidget *tree) const;
    /**
     * @brief 用目录数据填充目录选择树。
     * @param tree 目标树控件。
     * @param fileList 目录列表结果。
     * @param parentItem 为空时填充根层；非空时填充指定目录节点。
     */
    void populateDirectorySelectionTree(QTreeWidget *tree,
                                        const QList<NetworkFileInfo> &fileList,
                                        QTreeWidgetItem *parentItem = nullptr) const;
    /**
     * @brief 用目录数据填充下载页或目录页的文件列表树。
     * @param tree 目标树控件。
     * @param fileList 文件列表结果。
     */
    void populateDirectoryListingTree(QTreeWidget *tree,
                                      const QList<NetworkFileInfo> &fileList) const;

    QString getCurrentPath(const QString &nodeId) const;
    /**
     * @brief 记录指定节点当前正在浏览的路径。
     * @param nodeId 目标节点 ID。
     * @param path 当前目录路径。
     */
    void setCurrentPath(const QString &nodeId, const QString &path);
    /**
     * @brief 将字节数格式化为可读大小字符串。
     * @param size 文件大小，单位字节。
     * @return 格式化后的大小文案。
     */
    static QString formatFileSize(qint64 size);

signals:
    void fileListLoaded(const QString &nodeId, const QString &path);
    void loadError(const QString &nodeId, const QString &error);

private slots:
    /**
     * @brief 接收网络层目录更新信号，并转成页面层可消费的加载完成通知。
     * @param nodeId 发生更新的节点 ID。
     * @param fileList 当前目录结果，页面层不直接消费该参数。
     */
    void handleFileListUpdated(const QString &nodeId, const QList<NetworkFileInfo> &fileList);

private:
    /**
     * @brief 将目录结果转成最终树项展示。
     * @param tree 目标树控件。
     * @param fileList 文件列表结果。
     * @param nodeId 当前目录所属的节点 ID。
     */
    void fillTreeWidget(QTreeWidget *tree,
                        const QList<NetworkFileInfo> &fileList,
                        const QString &nodeId);

    DirectoryGateway *m_directoryGateway;
    NodeGateway *m_nodeGateway;
    QMap<QString, QString> m_currentPaths;
    QMap<QTreeWidget*, quint64> m_pendingRequests;
    quint64 m_requestCounter;
};

#endif
