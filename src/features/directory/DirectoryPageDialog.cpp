/* 新建目录对话框实现：只负责 Designer UI 的创建与销毁。 */
#include "DirectoryPageDialog.h"
#include "ui_DirectoryPageDialog.h"

#include <QComboBox>

DirectoryPageDialog::DirectoryPageDialog(QWidget *parent)
    : QDialog(parent)
    , m_nodeComboBox(new QComboBox(this))
    , ui(new Ui::DirectoryPageDialog)
{
    ui->setupUi(this);
    m_nodeComboBox->hide();
}

QComboBox *DirectoryPageDialog::nodeComboBox() const
{
    return m_nodeComboBox;
}

DirectoryPageDialog::~DirectoryPageDialog()
{
    delete ui;
}

QLineEdit *DirectoryPageDialog::pathEdit() const
{
    return ui->pathEdit;
}

QPushButton *DirectoryPageDialog::browseButton() const
{
    return ui->browseButton;
}

QPushButton *DirectoryPageDialog::refreshButton() const
{
    return ui->refreshButton;
}

QTreeWidget *DirectoryPageDialog::folderTree() const
{
    return ui->folderTree;
}

QPushButton *DirectoryPageDialog::cancelButton() const
{
    return ui->cancelButton;
}

QPushButton *DirectoryPageDialog::okButton() const
{
    return ui->okButton;
}
