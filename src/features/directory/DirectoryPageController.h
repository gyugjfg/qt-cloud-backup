#ifndef DIRECTORYPAGECONTROLLER_H
#define DIRECTORYPAGECONTROLLER_H

#include <QObject>

class DirectoryModule;
class DirectoryGateway;
class FileBrowser;
class NodeGateway;
class QComboBox;
class QTabWidget;
class QTreeWidgetItem;
class QWidget;

// 目录页控制层：统一处理默认页、动态页签和目录选择对话框之间的交互。
class DirectoryPageController : public QObject
{
    Q_OBJECT

public:
    struct DirectorySelectionResult {
        int nodeIndex = -1;
        QString nodeId;
        QString path;
    };

    explicit DirectoryPageController(QTabWidget *nodeTabWidget,
                                     QComboBox *downloadNodeComboBox,
                                     FileBrowser *fileBrowser,
                                     NodeGateway *nodeGateway,
                                     DirectoryGateway *directoryGateway,
                                     DirectoryModule *directoryModule,
                                     QWidget *messageParent,
                                     QObject *parent = nullptr);

    /**
     * @brief 打开新建目录页对话框，并在确认后创建或复用对应目录页签。
     * @param initialNodeIndex 初始节点下拉索引。
     */
    void openDirectoryPageDialog(int initialNodeIndex);
    /**
     * @brief 打开目录选择对话框并返回最终选择结果。
     * @param initialNodeIndex 初始节点下拉索引。
     * @param initialPath 初始目录路径。
     * @param result 返回的目录选择结果。
     * @param allowNodeSelection 兼容参数；当前实现保留节点选择器原有可用状态，不主动禁用切换。
     * @return 用户是否确认并得到有效结果。
     */
    bool openDirectorySelectionDialog(int initialNodeIndex,
                                      const QString &initialPath,
                                      DirectorySelectionResult &result,
                                      bool allowNodeSelection = true);
    void bindDirectoryInteractions(QWidget *tab);
    int findExistingNodeTabIndex(const QString &nodeId, const QString &path) const;
    bool activateExistingNodeTab(int tabIndex, const QString &nodeId, const QString &path);
    QWidget *createNodeDirectoryTab(const QString &nodeId, const QString &nodeName, const QString &dirPath);
    QString directoryTabNodeId(QWidget *tab, const QString &fallbackNodeId = QString()) const;
    QString directoryTabNodeName(QWidget *tab, const QString &fallbackNodeName = QString()) const;
    /**
     * @brief 驱动目录页切到指定节点和路径。
     * @param tab 目标页签。
     * @param nodeId 目标节点 ID。
     * @param nodeName 目标节点显示名。
     * @param path 目标路径。
     */
    void navigateDirectoryTab(QWidget *tab, const QString &nodeId, const QString &nodeName, const QString &path);
    void applyNodeTabPathState(QWidget *tab, const QString &nodeId, const QString &nodeName, const QString &path);
    void handleBreadcrumbNavigate(const QString &nodeId, const QString &path);

private:
    void setupDirectoryTabUi(QWidget *tab, const QString &nodeId, const QString &nodeName, const QString &dirPath);
    void bindDirectoryTabInteractions(QWidget *tab);
    void updateDirectoryTabNodeState(QWidget *tab, const QString &nodeId, const QString &nodeName);
    void refreshDirectoryTabOnlineState(QWidget *tab, const QString &nodeId) const;
    void populateDownloadNodeCombo(QComboBox *comboBox) const;
    bool ensureDialogDownloadNodeSelected(QWidget *dialogParent, QComboBox *nodeComboBox,
                                          QString &nodeId, QString *nodeName = nullptr) const;
    void applyDirectorySelectionResult(QComboBox *nodeComboBox, const QString &path,
                                       DirectorySelectionResult &result) const;

    QTabWidget *m_nodeTabWidget;
    QComboBox *m_downloadNodeComboBox;
    FileBrowser *m_fileBrowser;
    NodeGateway *m_nodeGateway;
    DirectoryGateway *m_directoryGateway;
    DirectoryModule *m_directoryModule;
    QWidget *m_messageParent;
};

#endif // DIRECTORYPAGECONTROLLER_H
