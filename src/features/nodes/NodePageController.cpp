/**
 * @file NodePageController.cpp
 * @brief 默认节点页展示和目录入口联动实现。
 *
 * 本控制器只操作已注入的页面对象，不创建或释放页面；所有方法预期在
 * GUI 线程执行，缺少可选控件时直接跳过对应展示更新。
 */
#include "NodePageController.h"

#include "DirectoryPageController.h"
#include "FileBrowser.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QTreeWidget>

NodePageController::NodePageController(QTabWidget *nodeTabWidget,
                                       QWidget *defaultNodePage,
                                       FileBrowser *fileBrowser,
                                       DirectoryPageController *directoryPageController,
                                       QObject *parent)
    : QObject(parent)
    , m_nodeTabWidget(nodeTabWidget)
    , m_defaultNodePage(defaultNodePage)
    , m_fileBrowser(fileBrowser)
    , m_directoryPageController(directoryPageController)
{
    // 默认节点页仍是固定页签，这里专门处理它和目录模块的页面联动。
}

void NodePageController::applyDefaultNodePageState(const QString &nodeName,
                                                   const QString &statusText,
                                                   const QString &statusStyle,
                                                   const QString &path,
                                                   bool clearFolderTree)
{
    // 旧调用方仍传入样式参数；当前样式由状态文本统一决定，避免页面出现两套颜色规则。
    Q_UNUSED(statusStyle);
    if (!m_defaultNodePage) {
        return;
    }

    m_defaultNodePage->setProperty("dirPath", path);

    if (QLabel *nodeNameLabel = m_defaultNodePage->findChild<QLabel*>("nodeNameLabel")) {
        nodeNameLabel->setText(nodeName);
    }
    if (QLabel *nodeStatusLabel = m_defaultNodePage->findChild<QLabel*>("nodeStatusLabel")) {
        nodeStatusLabel->setText(statusText);
        const bool online = statusText == QStringLiteral("在线");
        nodeStatusLabel->setStyleSheet(online
            ? "color: #07856F; font-size: 13px; font-weight: 600;"
            : "color: #C73B52; font-size: 13px; font-weight: 600;");
    }
    if (QLineEdit *pathEdit = m_defaultNodePage->findChild<QLineEdit*>("pathEdit")) {
        pathEdit->setText(path);
    }
    if (clearFolderTree) {
        if (QTreeWidget *folderTree = m_defaultNodePage->findChild<QTreeWidget*>("folderTree")) {
            folderTree->clear();
        }
    }
}

void NodePageController::resetDefaultNodePageDisplay()
{
    if (m_nodeTabWidget && m_defaultNodePage) {
        m_nodeTabWidget->setCurrentWidget(m_defaultNodePage);
        m_nodeTabWidget->setTabText(0, QStringLiteral("文件浏览"));
    }

    applyDefaultNodePageState(QStringLiteral("未选择节点"),
                              QStringLiteral("离线"),
                              "color: #ff0000; font-size: 14px;",
                              QStringLiteral("/"),
                              true);
}

void NodePageController::configureDefaultFolderTree()
{
    if (!m_defaultNodePage || !m_fileBrowser) {
        return;
    }

    QTreeWidget *folderTree = m_defaultNodePage->findChild<QTreeWidget*>("folderTree");
    m_fileBrowser->configureFileTree(folderTree);
}

void NodePageController::resetDefaultNodePage()
{
    resetDefaultNodePageDisplay();
}

/**
 * @brief 按当前节点选择刷新默认节点页的展示和目录状态。
 * @param nodeId 当前选中节点 ID。
 * @param nodeName 当前选中节点显示名。
 */
void NodePageController::refreshDefaultNodePageForSelection(const QString &nodeId, const QString &nodeName)
{
    if (m_nodeTabWidget && m_defaultNodePage) {
        m_nodeTabWidget->setCurrentWidget(m_defaultNodePage);
        m_nodeTabWidget->setTabText(0, QStringLiteral("文件浏览"));
    }

    if (m_fileBrowser) {
        const QString currentPath = m_fileBrowser->getCurrentPath(nodeId).isEmpty()
            ? QStringLiteral("/")
            : m_fileBrowser->getCurrentPath(nodeId);
        m_fileBrowser->setCurrentPath(nodeId, currentPath);
    }

    if (m_defaultNodePage) {
        m_defaultNodePage->setProperty("nodeId", nodeId);
        m_defaultNodePage->setProperty("dirPath", QStringLiteral("/"));
    }

    applyDefaultNodePageState(nodeName,
                              QStringLiteral("离线"),
                              "color: #ff0000; font-size: 14px;",
                              QStringLiteral("/"),
                              false);

    // 默认节点页既要切标题和路径，也要把目录浏览上下文同步给目录控制层。
    if (m_directoryPageController) {
        const QString currentPath = m_fileBrowser ? m_fileBrowser->getCurrentPath(nodeId) : QStringLiteral("/");
        m_directoryPageController->navigateDirectoryTab(m_defaultNodePage,
                                                        nodeId,
                                                        nodeName,
                                                        currentPath.isEmpty() ? QStringLiteral("/") : currentPath);
    }

    if (m_defaultNodePage) {
        if (QLineEdit *searchEdit = m_defaultNodePage->findChild<QLineEdit*>("searchEdit")) {
            searchEdit->clear();
        }
        if (QComboBox *filterComboBox = m_defaultNodePage->findChild<QComboBox*>("filterComboBox")) {
            filterComboBox->setCurrentIndex(0);
        }
    }
}
