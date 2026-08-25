#ifndef DIRECTORYSELECTIONDIALOG_H
#define DIRECTORYSELECTIONDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QPushButton;
class QTreeWidget;
namespace Ui {
class DirectorySelectionDialog;
}
QT_END_NAMESPACE

// 目录选择对话框外壳：控制器在外部接管节点切换、目录加载和确认流程。
class DirectorySelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DirectorySelectionDialog(QWidget *parent = nullptr);
    ~DirectorySelectionDialog();

    QComboBox *nodeComboBox() const;
    QLineEdit *searchEdit() const;
    QPushButton *searchButton() const;
    QTreeWidget *directoryTree() const;
    QPushButton *cancelButton() const;
    QPushButton *okButton() const;

private:
    QComboBox *m_nodeComboBox;
    Ui::DirectorySelectionDialog *ui;
};

#endif // DIRECTORYSELECTIONDIALOG_H
