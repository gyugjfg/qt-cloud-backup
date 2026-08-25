/* 目录选择对话框实现：交互流程由 DirectoryPageController 在外部编排。 */
#include "DirectorySelectionDialog.h"
#include "ui_DirectorySelectionDialog.h"

#include <QComboBox>

DirectorySelectionDialog::DirectorySelectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_nodeComboBox(new QComboBox(this))
    , ui(new Ui::DirectorySelectionDialog)
{
    ui->setupUi(this);
    m_nodeComboBox->hide();
}

QComboBox *DirectorySelectionDialog::nodeComboBox() const
{
    return m_nodeComboBox;
}

DirectorySelectionDialog::~DirectorySelectionDialog()
{
    delete ui;
}

QLineEdit *DirectorySelectionDialog::searchEdit() const
{
    return ui->searchEdit;
}

QPushButton *DirectorySelectionDialog::searchButton() const
{
    return ui->searchButton;
}

QTreeWidget *DirectorySelectionDialog::directoryTree() const
{
    return ui->dirTree;
}

QPushButton *DirectorySelectionDialog::cancelButton() const
{
    return ui->dirDialogCancelButton;
}

QPushButton *DirectorySelectionDialog::okButton() const
{
    return ui->dirDialogOkButton;
}
