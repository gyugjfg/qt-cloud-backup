#ifndef DIRECTORYPAGEDIALOG_H
#define DIRECTORYPAGEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QPushButton;
class QTreeWidget;
namespace Ui {
class DirectoryPageDialog;
}
QT_END_NAMESPACE

// 新建目录页对话框外壳：只承接固定 UI，不承接目录业务逻辑。
class DirectoryPageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DirectoryPageDialog(QWidget *parent = nullptr);
    ~DirectoryPageDialog();

    QComboBox *nodeComboBox() const;
    QLineEdit *pathEdit() const;
    QPushButton *browseButton() const;
    QPushButton *refreshButton() const;
    QTreeWidget *folderTree() const;
    QPushButton *cancelButton() const;
    QPushButton *okButton() const;

private:
    QComboBox *m_nodeComboBox;
    Ui::DirectoryPageDialog *ui;
};

#endif // DIRECTORYPAGEDIALOG_H
