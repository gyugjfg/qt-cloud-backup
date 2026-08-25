#ifndef FILETYPEPOLICY_H
#define FILETYPEPOLICY_H

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

// 目录文件类型规则的唯一入口：筛选和列表展示只共享规则，不读取网络或控件状态。
namespace FileTypePolicy {

inline QString normalizedName(const QString &fileName)
{
    return QFileInfo(fileName).fileName().trimmed().toLower();
}

inline bool hasSuffix(const QString &name, const QStringList &suffixes)
{
    for (const QString &suffix : suffixes) {
        if (name.endsWith(suffix)) {
            return true;
        }
    }
    return false;
}

inline bool isTextFile(const QString &fileName)
{
    return hasSuffix(normalizedName(fileName), {
        QStringLiteral(".txt"), QStringLiteral(".doc"), QStringLiteral(".docx"),
        QStringLiteral(".pdf"), QStringLiteral(".log"), QStringLiteral(".md"),
        QStringLiteral(".csv"), QStringLiteral(".json"), QStringLiteral(".xml"),
        QStringLiteral(".yaml"), QStringLiteral(".yml"), QStringLiteral(".conf"),
        QStringLiteral(".cfg"), QStringLiteral(".ini"), QStringLiteral(".sh"),
        QStringLiteral(".bash"), QStringLiteral(".bat"), QStringLiteral(".cmd"),
        QStringLiteral(".py"), QStringLiteral(".c"), QStringLiteral(".cc"),
        QStringLiteral(".cpp"), QStringLiteral(".h"), QStringLiteral(".hpp"),
        QStringLiteral(".java"), QStringLiteral(".js"), QStringLiteral(".ts"),
        QStringLiteral(".html"), QStringLiteral(".css"), QStringLiteral(".sql"),
        QStringLiteral(".rtf"), QStringLiteral(".toml")
    });
}

inline bool isImageFile(const QString &fileName)
{
    return hasSuffix(normalizedName(fileName), {
        QStringLiteral(".jpg"), QStringLiteral(".jpeg"), QStringLiteral(".png"),
        QStringLiteral(".gif"), QStringLiteral(".bmp"), QStringLiteral(".svg"),
        QStringLiteral(".webp"), QStringLiteral(".ico"), QStringLiteral(".tiff"),
        QStringLiteral(".tif"), QStringLiteral(".psd")
    });
}

inline bool isAudioFile(const QString &fileName)
{
    return hasSuffix(normalizedName(fileName), {
        QStringLiteral(".mp3"), QStringLiteral(".wav"), QStringLiteral(".flac"),
        QStringLiteral(".aac"), QStringLiteral(".ogg"), QStringLiteral(".wma"),
        QStringLiteral(".m4a"), QStringLiteral(".opus"), QStringLiteral(".ape")
    });
}

inline bool isVideoFile(const QString &fileName)
{
    return hasSuffix(normalizedName(fileName), {
        QStringLiteral(".mp4"), QStringLiteral(".avi"), QStringLiteral(".mkv"),
        QStringLiteral(".mov"), QStringLiteral(".wmv"), QStringLiteral(".flv"),
        QStringLiteral(".webm"), QStringLiteral(".rmvb"), QStringLiteral(".ts"),
        QStringLiteral(".m4v")
    });
}

inline bool isArchiveFile(const QString &fileName)
{
    return hasSuffix(normalizedName(fileName), {
        QStringLiteral(".zip"), QStringLiteral(".rar"), QStringLiteral(".7z"),
        QStringLiteral(".tar"), QStringLiteral(".gz"), QStringLiteral(".bz2"),
        QStringLiteral(".xz"), QStringLiteral(".tar.gz"), QStringLiteral(".tgz"),
        QStringLiteral(".tar.bz2"), QStringLiteral(".lz4"), QStringLiteral(".zst")
    });
}

inline bool isKnownExecutableDirectory(const QString &filePath)
{
    const QString path = QDir::cleanPath(filePath).toLower();
    for (const QString &directory : {
             QStringLiteral("/bin"), QStringLiteral("/sbin"),
             QStringLiteral("/usr/bin"), QStringLiteral("/usr/sbin"),
             QStringLiteral("/usr/local/bin"), QStringLiteral("/usr/local/sbin")
         }) {
        if (path.startsWith(directory + QStringLiteral("/"))) {
            return true;
        }
    }
    return false;
}

inline bool isExecutableFile(const QString &fileName, const QString &filePath = QString())
{
    const QString name = normalizedName(fileName);
    if (hasSuffix(name, {
            QStringLiteral(".exe"), QStringLiteral(".dll"), QStringLiteral(".msi"),
            QStringLiteral(".deb"), QStringLiteral(".rpm"), QStringLiteral(".appimage"),
            QStringLiteral(".bin"), QStringLiteral(".run"), QStringLiteral(".out"),
            QStringLiteral(".elf"), QStringLiteral(".so"), QStringLiteral(".a"),
            QStringLiteral(".dylib"), QStringLiteral(".com"), QStringLiteral(".cmd"),
            QStringLiteral(".bat"), QStringLiteral(".ps1"), QStringLiteral(".sh")
        })) {
        return true;
    }

    // 现有协议只传文件名、大小和目录标志，没有传 Unix mode；对无后缀文件
    // 只能依据标准可执行目录做保守识别，避免把普通无后缀文档全部误判为程序。
    return QFileInfo(name).suffix().isEmpty() && isKnownExecutableDirectory(filePath);
}

inline bool matches(const QString &fileName,
                    bool isDirectory,
                    const QString &filterType,
                    const QString &filePath = QString())
{
    if (filterType == QStringLiteral("全部文件")) {
        return true;
    }
    if (filterType == QStringLiteral("仅目录")) {
        return isDirectory;
    }
    if (isDirectory) {
        return false;
    }
    if (filterType == QStringLiteral("文本文件")) {
        return isTextFile(fileName);
    }
    if (filterType == QStringLiteral("图片文件")) {
        return isImageFile(fileName);
    }
    if (filterType == QStringLiteral("音频文件")) {
        return isAudioFile(fileName);
    }
    if (filterType == QStringLiteral("视频文件")) {
        return isVideoFile(fileName);
    }
    if (filterType == QStringLiteral("压缩文件")) {
        return isArchiveFile(fileName);
    }
    if (filterType == QStringLiteral("仅可执行文件")
        || filterType == QStringLiteral("可执行文件")) {
        return isExecutableFile(fileName, filePath);
    }
    return true;
}

inline QString displayType(const QString &fileName,
                           bool isDirectory,
                           const QString &filePath = QString())
{
    if (isDirectory) {
        return QStringLiteral("目录");
    }
    if (isExecutableFile(fileName, filePath)) {
        return QStringLiteral("可执行文件");
    }
    if (isTextFile(fileName)) {
        return QStringLiteral("文本文件");
    }
    if (isImageFile(fileName)) {
        return QStringLiteral("图片文件");
    }
    if (isAudioFile(fileName)) {
        return QStringLiteral("音频文件");
    }
    if (isVideoFile(fileName)) {
        return QStringLiteral("视频文件");
    }
    if (isArchiveFile(fileName)) {
        return QStringLiteral("压缩文件");
    }
    const QString suffix = QFileInfo(normalizedName(fileName)).suffix();
    return suffix.isEmpty() ? QStringLiteral("文件") : suffix.toUpper() + QStringLiteral(" 文件");
}

} // namespace FileTypePolicy

#endif // FILETYPEPOLICY_H
