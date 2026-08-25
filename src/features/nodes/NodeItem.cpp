/**
 * @file NodeItem.cpp
 * @brief 节点列表单行控件的显示和勾选信号实现。
 *
 * 行控件不查询节点服务，也不维护全局选择集合；列表层通过
 * checkStatusChanged 收口勾选状态。
 */
#include "NodeItem.h"
#include "ui_NodeItem.h"

NodeItem::NodeItem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NodeItem)
    , itemIndex(0)
{
    ui->setupUi(this);

    connect(ui->checkBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        onCheckBoxStateChanged(static_cast<int>(state));
    });
}

NodeItem::~NodeItem()
{
    delete ui;
}

void NodeItem::setNodeName(const QString &name)
{
    ui->nodeNameLabel->setText(name);
}

void NodeItem::setNodeIP(const QString &ip)
{
    ui->nodeIPLabel->setText("IP: " + ip);
}

void NodeItem::setNodePort(const QString &port)
{
    ui->nodePortLabel->setText("端口: " + port);
}

void NodeItem::setCheckStatus(Qt::CheckState state)
{
    ui->checkBox->setCheckState(state);
}

void NodeItem::setItemIndex(int index)
{
    itemIndex = index;
}

QString NodeItem::getNodeName() const
{
    return ui->nodeNameLabel->text();
}

QString NodeItem::getNodeIP() const
{
    return ui->nodeIPLabel->text().remove(0, 4);
}

QString NodeItem::getNodePort() const
{
    return ui->nodePortLabel->text().remove(0, 4);
}

bool NodeItem::isChecked() const
{
    return ui->checkBox->isChecked();
}

int NodeItem::getItemIndex() const
{
    return itemIndex;
}

void NodeItem::onCheckBoxStateChanged(int state)
{
    emit checkStatusChanged(state == Qt::Checked, this);
}
