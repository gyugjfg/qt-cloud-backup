#ifndef DIRECTORYTABPAGE_H
#define DIRECTORYTABPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class DirectoryTabPage;
}
QT_END_NAMESPACE

// 动态目录页签的固定骨架，运行时交互继续由控制层填充。
class DirectoryTabPage : public QWidget
{
    Q_OBJECT

public:
    explicit DirectoryTabPage(QWidget *parent = nullptr);
    ~DirectoryTabPage();

private:
    Ui::DirectoryTabPage *ui;
};

#endif // DIRECTORYTABPAGE_H
