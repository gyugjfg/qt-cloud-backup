#ifndef DOWNLOADCONTROLLER_H
#define DOWNLOADCONTROLLER_H

#include <QObject>
#include <QList>
#include <QString>

class NetWork;
class DownloadModule;
class QComboBox;
class QTreeWidget;
class QWidget;

/**
 * @brief 下载页入口控制器。
 *
 * 控制器只把页面文件树交给 DownloadModule，并转发模块结果信号；节点
 * 可用性、保存目录和任务创建由模块处理。注入的控件、父窗口和模块均
 * 为借用指针，方法应在 GUI 线程调用。
 */
class DownloadController : public QObject
{
    Q_OBJECT

public:
    /** 注入页面依赖；downloadNodeCombo 保留现有组装签名，实际校验由模块完成。 */
    explicit DownloadController(QComboBox *downloadNodeCombo,
                                QWidget *messageParent,
                                DownloadModule *downloadModule,
                                QObject *parent = nullptr);

    /** 将当前文件树的勾选结果交给模块；模块缺失时安全返回。 */
    void prepareDownloads(QTreeWidget *treeWidget);

signals:
    /** 转发下载任务创建结果。 */
    void downloadTaskCreated(const QString &taskId,
                             const QString &fileName);
    /** 请求主页切换到任务页。 */
    void switchToTaskPageRequested();

private:
    QWidget *m_messageParent;       ///< 借用的消息框父窗口。
    DownloadModule *m_downloadModule; ///< 借用的下载模块。
};

#endif // DOWNLOADCONTROLLER_H
