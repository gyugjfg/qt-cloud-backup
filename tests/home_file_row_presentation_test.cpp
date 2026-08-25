#include "HomeFileRowPresentation.h"

#include <QtTest>

class HomeFileRowPresentationTest : public QObject
{
    Q_OBJECT

private slots:
    /** 文件扩展名和目录类型应映射为原主页使用的图标、类型及路径字段。 */
    void mapsFileAndDirectoryRows();
    /** 文件大小边界应保持主页原有单位和未知值文案。 */
    void formatsSizeBoundaries();
};

void HomeFileRowPresentationTest::mapsFileAndDirectoryRows()
{
    NetworkFileInfo file;
    file.fileName = QStringLiteral("photo.PNG");
    file.filePath = QStringLiteral("./photo.PNG");
    file.fileSize = 2048;
    file.modifyTime = QDateTime(QDate(2026, 7, 23), QTime(10, 11, 12));

    const HomeFileRowPresentation::Row fileRow = HomeFileRowPresentation::fromFileInfo(file);
    QCOMPARE(fileRow.displayName,
             QString::fromUtf8(u8"\U0001F5BC\ufe0f photo.PNG"));
    QCOMPARE(fileRow.sizeText, QStringLiteral("2.00 KB"));
    QCOMPARE(fileRow.modifiedText, QStringLiteral("2026-07-23 10:11:12"));
    QCOMPARE(fileRow.typeText, QString::fromUtf8(u8"\u6587\u4ef6"));
    QCOMPARE(fileRow.filePath, file.filePath);
    QVERIFY(!fileRow.isDirectory);

    NetworkFileInfo directory;
    directory.fileName = QString::fromUtf8(u8"\u6587\u6863");
    directory.filePath = QStringLiteral("./docs");
    directory.isDirectory = true;

    const HomeFileRowPresentation::Row directoryRow =
        HomeFileRowPresentation::fromFileInfo(directory);
    QCOMPARE(directoryRow.displayName,
             QString::fromUtf8(u8"\U0001F4C1 \u6587\u6863"));
    QCOMPARE(directoryRow.typeText, QString::fromUtf8(u8"\u76ee\u5f55"));
    QCOMPARE(directoryRow.sizeText, QString::fromUtf8(u8"\u672a\u77e5"));
    QVERIFY(directoryRow.isDirectory);
}

void HomeFileRowPresentationTest::formatsSizeBoundaries()
{
    QCOMPARE(HomeFileRowPresentation::formatFileSize(0), QString::fromUtf8(u8"\u672a\u77e5"));
    QCOMPARE(HomeFileRowPresentation::formatFileSize(-1), QString::fromUtf8(u8"\u672a\u77e5"));
    QCOMPARE(HomeFileRowPresentation::formatFileSize(1023), QStringLiteral("1023 B"));
    QCOMPARE(HomeFileRowPresentation::formatFileSize(1024), QStringLiteral("1.00 KB"));
    QCOMPARE(HomeFileRowPresentation::formatFileSize(1024 * 1024), QStringLiteral("1.00 MB"));
    QCOMPARE(HomeFileRowPresentation::formatFileSize(1024LL * 1024 * 1024),
             QStringLiteral("1.00 GB"));
}

QTEST_MAIN(HomeFileRowPresentationTest)
#include "home_file_row_presentation_test.moc"
