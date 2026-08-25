#ifndef NODEITEM_H
#define NODEITEM_H

#include <QWidget>

namespace Ui {
class NodeItem;
}

/**
 * @brief 节点列表中的单行展示控件。
 *
 * 该控件只保存当前行的显示值、索引和勾选状态。节点增删、在线状态
 * 和持久化由 NodeModule 负责；勾选变化通过信号交给列表层维护。
 * 控件由 QListWidget 的父对象管理，调用方不应单独 delete。
 */
class NodeItem : public QWidget
{
    Q_OBJECT

public:
    explicit NodeItem(QWidget *parent = nullptr);
    ~NodeItem();

    /** 更新行内节点名称。调用发生在 GUI 线程。 */
    void setNodeName(const QString &name);
    /** 更新行内 IP 展示文本，不执行地址校验。 */
    void setNodeIP(const QString &ip);
    /** 更新行内端口展示文本，不执行端口转换。 */
    void setNodePort(const QString &port);
    /** 更新勾选状态；内部信号是否触发由调用方的 QSignalBlocker 决定。 */
    void setCheckStatus(Qt::CheckState state);
    /** 设置列表层使用的稳定行索引。 */
    void setItemIndex(int index);

    QString getNodeName() const;
    QString getNodeIP() const;
    QString getNodePort() const;
    /** 返回当前是否为 Checked 状态。 */
    bool isChecked() const;
    /** 返回最近一次由列表层设置的行索引。 */
    int getItemIndex() const;

signals:
    void checkStatusChanged(bool status, QWidget *item);

private slots:
    void onCheckBoxStateChanged(int state);

private:
    Ui::NodeItem *ui;
    int itemIndex;
};

#endif // NODEITEM_H
