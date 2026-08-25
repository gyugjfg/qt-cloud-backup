#ifndef NODEPAGECONTROLLER_H
#define NODEPAGECONTROLLER_H

#include <QObject>

class DirectoryPageController;
class FileBrowser;
class QTabWidget;
class QWidget;

/**
 * @brief 默认节点页展示控制器。
 *
 * 该控制器只改默认页签、路径、状态标签和目录入口上下文；所有注入的
 * 页面、FileBrowser 与 DirectoryPageController 均为借用指针，生命周期由
 * 主页组合根保证。方法应在 GUI 线程调用，缺少依赖时以安全返回表示。
 */
class NodePageController : public QObject
{
    Q_OBJECT

public:
    explicit NodePageController(QTabWidget *nodeTabWidget,
                                QWidget *defaultNodePage,
                                FileBrowser *fileBrowser,
                                DirectoryPageController *directoryPageController,
                                QObject *parent = nullptr);

    /** 将默认节点页恢复为未选择节点的展示状态。 */
    void resetDefaultNodePageDisplay();
    /** 为默认页的目录树注入 FileBrowser 所需的控件配置。 */
    void configureDefaultFolderTree();
    /** resetDefaultNodePageDisplay 的兼容入口。 */
    void resetDefaultNodePage();
    /** 按当前节点刷新默认页展示，并通知目录控制器切换上下文。 */
    void refreshDefaultNodePageForSelection(const QString &nodeId, const QString &nodeName);

private:
    /** 写入默认页的通用展示状态；statusStyle 参数保留兼容语义，当前按在线文本统一设样式。 */
    void applyDefaultNodePageState(const QString &nodeName,
                                   const QString &statusText,
                                   const QString &statusStyle,
                                   const QString &path,
                                   bool clearFolderTree);

    QTabWidget *m_nodeTabWidget = nullptr;
    QWidget *m_defaultNodePage = nullptr;
    FileBrowser *m_fileBrowser = nullptr;
    DirectoryPageController *m_directoryPageController = nullptr;
};

#endif // NODEPAGECONTROLLER_H
