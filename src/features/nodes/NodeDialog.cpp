/**
 * @file NodeDialog.cpp
 * @brief 节点编辑表单的 GUI 实现。
 *
 * 本文件只负责表单回填、读取和最基本的非空校验；节点持久化和网络
 * 缓存同步由 NodeModule 在对话框 Accepted 后完成。
 */
#include "NodeDialog.h"
#include "ui_NodeDialog.h"
#include <QMessageBox>

NodeDialog::NodeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NodeDialog)
{
    ui->setupUi(this);
}

NodeDialog::~NodeDialog()
{
    delete ui;
}

void NodeDialog::setTitle(const QString &title)
{
    ui->titleLabel->setText(title);
    setWindowTitle(title);
}

void NodeDialog::setNodeInfo(const QString &name, const QString &ip, const QString &port)
{
    ui->nameEdit->setText(name);
    ui->ipEdit->setText(ip);

    // 历史数据里未设置端口时会落成 "0"，这里仍按空值展示给用户编辑。
    if (port == "0") {
        ui->portEdit->setText("");
    } else {
        ui->portEdit->setText(port);
    }
}

QString NodeDialog::getNodeName() const
{
    return ui->nameEdit->text();
}

QString NodeDialog::getNodeIP() const
{
    return ui->ipEdit->text();
}

QString NodeDialog::getNodePort() const
{
    return ui->portEdit->text();
}

void NodeDialog::on_okButton_clicked()
{
    QString nodeName = ui->nameEdit->text();
    QString ip = ui->ipEdit->text();
    QString port = ui->portEdit->text();
    
    if (nodeName.isEmpty() || ip.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, "警告", "请填写完整的节点信息");
        return;
    }
    
    accept();
}
