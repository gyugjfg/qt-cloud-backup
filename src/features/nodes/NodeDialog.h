#ifndef NODEDIALOG_H
#define NODEDIALOG_H

#include <QDialog>

namespace Ui {
class NodeDialog;
}

/**
 * @brief 节点编辑对话框，只负责收集和校验一组节点连接参数。
 *
 * 对话框不写入数据库、不更新节点缓存，也不拥有调用方传入的节点对象。
 * 调用方通常在 GUI 线程中通过 exec() 使用它，并在 Accepted 后读取表单值。
 */
class NodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NodeDialog(QWidget *parent = nullptr);
    ~NodeDialog();
    
    /** 设置窗口标题和对话框内的标题文本。 */
    void setTitle(const QString &title);
    /** 回填待编辑节点；端口字符串 "0" 会按空端口展示。 */
    void setNodeInfo(const QString &name, const QString &ip, const QString &port);
    /** 返回用户当前填写的节点名称，不做额外格式化。 */
    QString getNodeName() const;
    /** 返回用户当前填写的 IP 文本，不做网络可达性检查。 */
    QString getNodeIP() const;
    /** 返回用户当前填写的端口文本，不转换为整数。 */
    QString getNodePort() const;

private slots:
    void on_okButton_clicked();

private:
    Ui::NodeDialog *ui;
};

#endif // NODEDIALOG_H
