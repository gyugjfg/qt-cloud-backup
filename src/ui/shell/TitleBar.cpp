#include "TitleBar.h"
#include "ui_TitleBar.h"

#include <QMouseEvent>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TitleBar)
{
    // 标题栏保持纯框架 UI 职责，不感知主页里的业务对象。
    ui->setupUi(this);
}

TitleBar::~TitleBar()
{
    delete ui;
}

void TitleBar::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        isDrag = true;

        QWidget *pptr = static_cast<QWidget*>(parent());
        // 父窗口存在时优先拖父窗口，标题栏自身不维护独立窗口位置。
        if(!pptr)
            dVal = ev->globalPosition() - pos();
        else
            dVal = ev->globalPosition() - pptr->pos();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *ev)
{
    if(isDrag)
    {
        QWidget *pptr = static_cast<QWidget*>(parent());
        if(!pptr)
            move((ev->globalPosition()-dVal).toPoint());
        else
            pptr->move((ev->globalPosition()-dVal).toPoint());
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
        isDrag = false;
}

void TitleBar::on_pushButton_clicked()
{
    QWidget *pptr = static_cast<QWidget*>(parent());
    if(!pptr)close();
    else pptr->close();
}

