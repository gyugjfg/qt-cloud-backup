#include <QtTest>

#include "FileTypePolicy.h"

class DirectoryFileTypePolicyTest final : public QObject
{
    Q_OBJECT

private slots:
    void recognizesAllFilterFamilies();
    void recognizesExecutableSuffixesAndUnixBinPaths();
    void keepsDirectoriesOutOfFileFilters();
};

void DirectoryFileTypePolicyTest::recognizesAllFilterFamilies()
{
    QVERIFY(FileTypePolicy::matches("README.MD", false, QStringLiteral("文本文件")));
    QVERIFY(FileTypePolicy::matches("photo.PNG", false, QStringLiteral("图片文件")));
    QVERIFY(FileTypePolicy::matches("recording.FLAC", false, QStringLiteral("音频文件")));
    QVERIFY(FileTypePolicy::matches("movie.MKV", false, QStringLiteral("视频文件")));
    QVERIFY(FileTypePolicy::matches("archive.TAR.GZ", false, QStringLiteral("压缩文件")));
    QVERIFY(FileTypePolicy::matches("anything.bin", false, QStringLiteral("全部文件")));
    QVERIFY(FileTypePolicy::matches("folder", true, QStringLiteral("仅目录")));
}

void DirectoryFileTypePolicyTest::recognizesExecutableSuffixesAndUnixBinPaths()
{
    for (const QString &fileName : {
             QStringLiteral("tool.EXE"), QStringLiteral("library.SO"),
             QStringLiteral("package.deb"), QStringLiteral("installer.AppImage"),
             QStringLiteral("script.sh")
         }) {
        QVERIFY2(FileTypePolicy::matches(fileName, false, QStringLiteral("可执行文件")),
                 qPrintable(fileName));
    }

    QVERIFY(FileTypePolicy::matches("bash", false,
                                    QStringLiteral("可执行文件"),
                                    QStringLiteral("/usr/bin/bash")));
    QVERIFY(!FileTypePolicy::matches("notes", false,
                                     QStringLiteral("可执行文件"),
                                     QStringLiteral("/home/user/notes")));
}

void DirectoryFileTypePolicyTest::keepsDirectoriesOutOfFileFilters()
{
    QVERIFY(!FileTypePolicy::matches("tool.exe", true, QStringLiteral("可执行文件")));
    QVERIFY(!FileTypePolicy::matches("photo.png", true, QStringLiteral("图片文件")));
    QCOMPARE(FileTypePolicy::displayType("tool.exe", false), QStringLiteral("可执行文件"));
    QCOMPARE(FileTypePolicy::displayType("folder", true), QStringLiteral("目录"));
}

QTEST_APPLESS_MAIN(DirectoryFileTypePolicyTest)

#include "directory_file_type_policy_test.moc"
