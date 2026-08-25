#ifndef FILEITEM_H
#define FILEITEM_H

#include <QWidget>
#include <QtCore>

class QLabel;
class QResizeEvent;

namespace Ui {
class FileItem;
}

// 上传页动态文件项：负责文件展示、勾选状态和原始路径保留。
class FileItem : public QWidget
{
    Q_OBJECT

public:
    struct UploadColumnMetrics {
        int leftMargin = 10;
        int rightMargin = 4;
        int spacing = 18;
        int sizePathGap = 28;
        int checkWidth = 24;
        int nameWidth = 290;
        int typeWidth = 108;
        int sizeWidth = 96;
        int pathWidth = 300;
    };

    struct UploadColumnGeometry {
        int nameX = 0;
        int typeX = 0;
        int sizeX = 0;
        int pathX = 0;
        int nameWidth = 0;
        int typeWidth = 0;
        int sizeWidth = 0;
        int pathWidth = 0;
    };

    explicit FileItem(QWidget *parent = nullptr);
    ~FileItem();

    static UploadColumnMetrics resolveColumnMetrics(int rowWidth);
    static void applyHeaderColumnMetrics(QLabel *nameLabel,
                                         QLabel *typeLabel,
                                         QLabel *sizeLabel,
                                         QLabel *pathLabel,
                                         int rowWidth);

    void applyColumnMetrics(const UploadColumnMetrics &metrics);
    UploadColumnGeometry currentColumnGeometry() const;
    void setFileName(QString name);
    void setFileType(QString type, QString fileName = "");
    void setFileSize(qint64 size);
    void setFilePath(QString path);
    void setItemIndex(int index);

    QString getFileName();
    QString getFileType();
    qint64 getFileSize();
    QString getFilePath();
    int getItemIndex();

    void setCheckStatus(Qt::CheckState status);
    Qt::CheckState getCheckStatus();

signals:
    void checkStatusChanged(bool checked, QWidget *item);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_CheckStatus_checkStateChanged(const Qt::CheckState &arg1);

private:
    void refreshDisplayTexts();
    QString elideText(const QLabel *label, const QString &text, Qt::TextElideMode mode) const;

    Ui::FileItem *ui;
    int itemIndex;
    qint64 fileSize;
    QString m_fileName;
    QString m_fileType;
    QString m_fileSizeText;
    QString m_filePath;
};

#endif // FILEITEM_H
