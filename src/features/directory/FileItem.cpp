#include "FileItem.h"
#include "ui_FileItem.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <utility>

namespace {
constexpr int kDefaultNameWidth = 290;
constexpr int kDefaultTypeWidth = 108;
constexpr int kDefaultSizeWidth = 96;
constexpr int kDefaultPathWidth = 300;
constexpr int kCompactTypeWidth = 96;
constexpr int kCompactSizeWidth = 92;
constexpr int kMinimumNameWidth = 132;
constexpr int kMinimumPathWidth = 150;
constexpr int kMinimumTypeWidth = 70;
constexpr int kMinimumSizeWidth = 72;
}

FileItem::UploadColumnMetrics FileItem::resolveColumnMetrics(int rowWidth)
{
    UploadColumnMetrics metrics;

    const int contentWidth = qMax(rowWidth - metrics.leftMargin - metrics.rightMargin, 0);
    const int layoutSpacingWidth = metrics.spacing * 4;
    const int baseFixedWidth = metrics.checkWidth + kDefaultNameWidth + kDefaultTypeWidth + kDefaultSizeWidth;
    const int baseRequiredWidth = baseFixedWidth + kDefaultPathWidth + layoutSpacingWidth;

    if (contentWidth <= 0) {
        return metrics;
    }

    if (contentWidth >= baseRequiredWidth) {
        metrics.nameWidth = kDefaultNameWidth;
        metrics.typeWidth = kDefaultTypeWidth;
        metrics.sizeWidth = kDefaultSizeWidth;
        metrics.pathWidth = qMax(kDefaultPathWidth, contentWidth - baseFixedWidth - layoutSpacingWidth);
        return metrics;
    }

    metrics.typeWidth = contentWidth >= 700 ? 104 : kCompactTypeWidth;
    metrics.sizeWidth = contentWidth >= 700 ? 92 : kCompactSizeWidth;

    int remainingWidth = qMax(0, contentWidth - metrics.checkWidth - metrics.typeWidth - metrics.sizeWidth - layoutSpacingWidth);
    metrics.nameWidth = qBound(kMinimumNameWidth,
                               static_cast<int>(remainingWidth * 0.48),
                               kDefaultNameWidth);
    metrics.pathWidth = qMax(kMinimumPathWidth, remainingWidth - metrics.nameWidth);

    auto reduceWidth = [](int &value, int minimum, int &overflow) {
        if (overflow <= 0 || value <= minimum) {
            return;
        }

        const int shrink = qMin(overflow, value - minimum);
        value -= shrink;
        overflow -= shrink;
    };

    int totalUsedWidth = metrics.checkWidth + metrics.nameWidth + metrics.typeWidth + metrics.sizeWidth + metrics.pathWidth + layoutSpacingWidth;
    int overflow = totalUsedWidth - contentWidth;
    if (overflow > 0) {
        reduceWidth(metrics.pathWidth, kMinimumPathWidth, overflow);
        reduceWidth(metrics.nameWidth, kMinimumNameWidth, overflow);
        reduceWidth(metrics.typeWidth, kMinimumTypeWidth, overflow);
        reduceWidth(metrics.sizeWidth, kMinimumSizeWidth, overflow);
    }

    const int flexibleWidth = qMax(0, remainingWidth - metrics.nameWidth - metrics.pathWidth);
    if (flexibleWidth > 0) {
        metrics.pathWidth += flexibleWidth;
    }

    totalUsedWidth = metrics.checkWidth + metrics.nameWidth + metrics.typeWidth + metrics.sizeWidth + metrics.pathWidth + layoutSpacingWidth;
    if (totalUsedWidth < contentWidth) {
        metrics.pathWidth += contentWidth - totalUsedWidth;
    }

    return metrics;
}

void FileItem::applyHeaderColumnMetrics(QLabel *nameLabel,
                                        QLabel *typeLabel,
                                        QLabel *sizeLabel,
                                        QLabel *pathLabel,
                                        int rowWidth)
{
    if (!nameLabel || !typeLabel || !sizeLabel || !pathLabel) {
        return;
    }

    const UploadColumnMetrics metrics = resolveColumnMetrics(rowWidth);

    nameLabel->setMinimumWidth(metrics.nameWidth);
    nameLabel->setMaximumWidth(metrics.nameWidth);
    typeLabel->setMinimumWidth(metrics.typeWidth);
    typeLabel->setMaximumWidth(metrics.typeWidth);
    sizeLabel->setMinimumWidth(metrics.sizeWidth);
    sizeLabel->setMaximumWidth(metrics.sizeWidth);
    pathLabel->setMinimumWidth(metrics.pathWidth);
    pathLabel->setMaximumWidth(metrics.pathWidth);
}

FileItem::FileItem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FileItem)
    , itemIndex(-1)
    , fileSize(0)
{
    // 上传列表展示文本可以压缩，但真实路径始终单独保留在 item 内部。
    ui->setupUi(this);
    applyColumnMetrics(resolveColumnMetrics(width()));
}

FileItem::~FileItem()
{
    delete ui;
}

void FileItem::setFileName(QString name)
{
    m_fileName = std::move(name);
    refreshDisplayTexts();
}

void FileItem::setFileType(QString type, QString fileName)
{
    QString chineseType;
    if (type.isEmpty()) {
        if (fileName.compare("Makefile", Qt::CaseInsensitive) == 0) {
            chineseType = "Makefile";
        } else if (fileName.compare("Makefile.Debug", Qt::CaseInsensitive) == 0) {
            chineseType = "Makefile.Debug";
        } else if (fileName.compare("Makefile.Release", Qt::CaseInsensitive) == 0) {
            chineseType = "Makefile.Release";
        } else {
            chineseType = "未知文件";
        }
    } else if (type.compare("h", Qt::CaseInsensitive) == 0) {
        chineseType = "C Header 源文件";
    } else if (type.compare("cpp", Qt::CaseInsensitive) == 0) {
        chineseType = "C++ 源文件";
    } else if (type.compare("ui", Qt::CaseInsensitive) == 0) {
        chineseType = "Qt UI file";
    } else if (type.compare("png", Qt::CaseInsensitive) == 0) {
        chineseType = "PNG 文件";
    } else if (type.compare("jpg", Qt::CaseInsensitive) == 0 || type.compare("jpeg", Qt::CaseInsensitive) == 0) {
        chineseType = "JPEG 文件";
    } else if (type.compare("txt", Qt::CaseInsensitive) == 0) {
        chineseType = "文本文件";
    } else if (type.compare("exe", Qt::CaseInsensitive) == 0) {
        chineseType = "可执行文件";
    } else if (type.compare("dll", Qt::CaseInsensitive) == 0) {
        chineseType = "动态链接库";
    } else if (type.compare("lib", Qt::CaseInsensitive) == 0) {
        chineseType = "静态库文件";
    } else if (type.compare("pro", Qt::CaseInsensitive) == 0) {
        chineseType = "Qt 项目文件";
    } else if (type.compare("pri", Qt::CaseInsensitive) == 0) {
        chineseType = "Qt 项目包含文件";
    } else if (type.compare("qrc", Qt::CaseInsensitive) == 0) {
        chineseType = "QRC 文件";
    } else if (type.compare("Release", Qt::CaseInsensitive) == 0) {
        chineseType = "Release";
    } else if (type.compare("Debug", Qt::CaseInsensitive) == 0) {
        chineseType = "Debug";
    } else {
        chineseType = type;
    }

    m_fileType = chineseType;
    refreshDisplayTexts();
}

void FileItem::setFileSize(qint64 size)
{
    fileSize = size;
    QString sizeStr;
    if (size < 1024) {
        sizeStr = QString::number(size) + " B";
    } else if (size < 1024 * 1024) {
        sizeStr = QString::number(size / 1024.0, 'f', 2) + " KB";
    } else if (size < 1024 * 1024 * 1024) {
        sizeStr = QString::number(size / (1024.0 * 1024), 'f', 2) + " MB";
    } else {
        sizeStr = QString::number(size / (1024.0 * 1024 * 1024), 'f', 2) + " GB";
    }
    m_fileSizeText = sizeStr;
    refreshDisplayTexts();
}

void FileItem::setFilePath(QString path)
{
    m_filePath = path;

    // 页面只做压缩显示，控制层后续仍通过原始路径创建上传任务。
    ui->FilePath->setToolTip("存储路径:" + path);
    refreshDisplayTexts();
}

void FileItem::setItemIndex(int index)
{
    itemIndex = index;
}

QString FileItem::getFileName()
{
    return m_fileName;
}

QString FileItem::getFileType()
{
    return m_fileType;
}

qint64 FileItem::getFileSize()
{
    return fileSize;
}

QString FileItem::getFilePath()
{
    return m_filePath;
}

int FileItem::getItemIndex()
{
    return itemIndex;
}

void FileItem::setCheckStatus(Qt::CheckState status)
{
    ui->CheckStatus->setCheckState(status);
}

Qt::CheckState FileItem::getCheckStatus()
{
    return ui->CheckStatus->checkState();
}

void FileItem::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshDisplayTexts();
}

void FileItem::on_CheckStatus_checkStateChanged(const Qt::CheckState &/*arg1*/)
{
    emit checkStatusChanged(ui->CheckStatus->isChecked(), this);
}

void FileItem::applyColumnMetrics(const UploadColumnMetrics &metrics)
{
    if (QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(this->layout())) {
        layout->setSpacing(metrics.spacing);
        layout->setContentsMargins(metrics.leftMargin, 8, metrics.rightMargin, 8);
    }

    ui->CheckStatus->setMinimumWidth(metrics.checkWidth);
    ui->CheckStatus->setMaximumWidth(metrics.checkWidth);

    ui->FileName->setMinimumWidth(metrics.nameWidth);
    ui->FileName->setMaximumWidth(metrics.nameWidth);

    ui->FileType->setMinimumWidth(metrics.typeWidth);
    ui->FileType->setMaximumWidth(metrics.typeWidth);

    ui->FileSize->setMinimumWidth(metrics.sizeWidth);
    ui->FileSize->setMaximumWidth(metrics.sizeWidth);

    ui->FilePath->setMinimumWidth(metrics.pathWidth);
    ui->FilePath->setMaximumWidth(metrics.pathWidth);

    refreshDisplayTexts();
}

FileItem::UploadColumnGeometry FileItem::currentColumnGeometry() const
{
    UploadColumnGeometry geometry;
    if (!ui) {
        return geometry;
    }

    geometry.nameX = ui->FileName->geometry().x();
    geometry.typeX = ui->FileType->geometry().x();
    geometry.sizeX = ui->FileSize->geometry().x();
    geometry.pathX = ui->FilePath->geometry().x();

    geometry.nameWidth = ui->FileName->width();
    geometry.typeWidth = ui->FileType->width();
    geometry.sizeWidth = ui->FileSize->width();
    geometry.pathWidth = ui->FilePath->width();
    return geometry;
}

void FileItem::refreshDisplayTexts()
{
    if (!ui) {
        return;
    }

    ui->FileName->setText(elideText(ui->FileName, m_fileName, Qt::ElideRight));
    ui->FileName->setToolTip(m_fileName);

    ui->FileType->setText(elideText(ui->FileType, m_fileType, Qt::ElideRight));
    ui->FileType->setToolTip(m_fileType);

    ui->FileSize->setText(m_fileSizeText);
    ui->FileSize->setToolTip(m_fileSizeText);

    ui->FilePath->setWordWrap(false);
    ui->FilePath->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->FilePath->setText(elideText(ui->FilePath, m_filePath, Qt::ElideMiddle));
}

QString FileItem::elideText(const QLabel *label, const QString &text, Qt::TextElideMode mode) const
{
    if (!label || text.isEmpty()) {
        return text;
    }

    const int availableWidth = qMax(label->width(), label->minimumWidth()) - 4;
    if (availableWidth <= 0) {
        return text;
    }

    const QFontMetrics metrics(label->font());
    return metrics.elidedText(text, mode, availableWidth);
}
