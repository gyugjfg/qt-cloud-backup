/**
 * @brief HomeWidge 下载树文件行展示映射的实现。
 *
 * 这里仅处理 NetworkFileInfo 到可展示值的同步转换，不触碰树控件或网络对象。
 */
#include "HomeFileRowPresentation.h"

namespace {

QString iconForFile(const QString &fileName, bool isDirectory)
{
    if (isDirectory) {
        return QString::fromUtf8(u8"\U0001F4C1 ");
    }

    const QString extension = fileName.section(QStringLiteral("."), -1).toLower();
    if (extension == QStringLiteral("txt")) {
        return QString::fromUtf8(u8"\U0001F4C4 ");
    }
    if (extension == QStringLiteral("pdf")) {
        return QString::fromUtf8(u8"\U0001F4D5 ");
    }
    if (extension == QStringLiteral("jpg") || extension == QStringLiteral("jpeg")
        || extension == QStringLiteral("png") || extension == QStringLiteral("gif")) {
        return QString::fromUtf8(u8"\U0001F5BC\ufe0f ");
    }
    if (extension == QStringLiteral("mp3") || extension == QStringLiteral("wav")
        || extension == QStringLiteral("flac")) {
        return QString::fromUtf8(u8"\U0001F3B5 ");
    }
    if (extension == QStringLiteral("mp4") || extension == QStringLiteral("avi")
        || extension == QStringLiteral("mkv")) {
        return QString::fromUtf8(u8"\U0001F3AC ");
    }
    if (extension == QStringLiteral("zip") || extension == QStringLiteral("rar")
        || extension == QStringLiteral("7z")) {
        return QString::fromUtf8(u8"\U0001F4E6 ");
    }
    if (extension == QStringLiteral("exe") || extension == QStringLiteral("dll")) {
        return QString::fromUtf8(u8"\U0001F4BB ");
    }
    return QString::fromUtf8(u8"\U0001F4C4 ");
}

} // namespace

HomeFileRowPresentation::Row HomeFileRowPresentation::fromFileInfo(const NetworkFileInfo &fileInfo)
{
    Row row;
    row.displayName = iconForFile(fileInfo.fileName, fileInfo.isDirectory) + fileInfo.fileName;
    row.sizeText = formatFileSize(fileInfo.fileSize);
    row.modifiedText = fileInfo.modifyTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    row.typeText = fileInfo.isDirectory ? QStringLiteral("目录") : QStringLiteral("文件");
    row.filePath = fileInfo.filePath;
    row.isDirectory = fileInfo.isDirectory;
    return row;
}

QString HomeFileRowPresentation::formatFileSize(qint64 size)
{
    if (size <= 0) {
        return QString::fromUtf8(u8"\u672a\u77e5");
    }
    if (size < 1024) {
        return QString::number(size) + QStringLiteral(" B");
    }
    if (size < 1024 * 1024) {
        return QString::number(size / 1024.0, 'f', 2) + QStringLiteral(" KB");
    }
    if (size < 1024 * 1024 * 1024) {
        return QString::number(size / (1024.0 * 1024), 'f', 2) + QStringLiteral(" MB");
    }
    return QString::number(size / (1024.0 * 1024 * 1024), 'f', 2) + QStringLiteral(" GB");
}
