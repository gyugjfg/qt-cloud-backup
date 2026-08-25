/* 动态目录页签实现：只创建固定控件骨架，不持有网络请求逻辑。 */
#include "DirectoryTabPage.h"
#include "ui_DirectoryTabPage.h"

DirectoryTabPage::DirectoryTabPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DirectoryTabPage)
{
    ui->setupUi(this);
}

DirectoryTabPage::~DirectoryTabPage()
{
    delete ui;
}
