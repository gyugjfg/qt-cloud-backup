#include <QtTest>

#include "DirectoryTabPresentation.h"

class DirectoryTabPresentationTest : public QObject
{
    Q_OBJECT

private slots:
    void titleKeepsExistingPathSuffixRules();
    void nodeNameKeepsExistingTitleParsingRule();
};

void DirectoryTabPresentationTest::titleKeepsExistingPathSuffixRules()
{
    QCOMPARE(DirectoryTabPresentation::title(QStringLiteral("工作节点"), QStringLiteral("/")),
             QStringLiteral("工作节点"));
    QCOMPARE(DirectoryTabPresentation::title(QStringLiteral("工作节点"), QString()),
             QStringLiteral("工作节点"));
    QCOMPARE(DirectoryTabPresentation::title(QStringLiteral("工作节点"), QStringLiteral("/documents")),
             QStringLiteral("工作节点 - documents"));
    QCOMPARE(DirectoryTabPresentation::title(QStringLiteral("工作节点"), QStringLiteral("/documents/")),
             QStringLiteral("工作节点"));
    QCOMPARE(DirectoryTabPresentation::title(QStringLiteral("节点 - A"), QStringLiteral("/reports")),
             QStringLiteral("节点 - A - reports"));
}

void DirectoryTabPresentationTest::nodeNameKeepsExistingTitleParsingRule()
{
    QCOMPARE(DirectoryTabPresentation::nodeName(QStringLiteral("工作节点")), QStringLiteral("工作节点"));
    QCOMPARE(DirectoryTabPresentation::nodeName(QStringLiteral("工作节点 - documents")),
             QStringLiteral("工作节点"));
    QCOMPARE(DirectoryTabPresentation::nodeName(QStringLiteral("节点 - A - reports")), QStringLiteral("节点"));
}

QTEST_APPLESS_MAIN(DirectoryTabPresentationTest)

#include "directory_tab_presentation_test.moc"
