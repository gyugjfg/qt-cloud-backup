#include <QtTest>

#include "DirectoryPathNavigation.h"

class DirectoryPathNavigationTest : public QObject
{
    Q_OBJECT

private slots:
    void parentPathKeepsExistingRules();
    void breadcrumbsBuildCumulativePaths();
};

void DirectoryPathNavigationTest::parentPathKeepsExistingRules()
{
    QString result = QStringLiteral("unchanged");
    QVERIFY(!DirectoryPathNavigation::parentPath(QString(), result));
    QCOMPARE(result, QStringLiteral("unchanged"));
    QVERIFY(!DirectoryPathNavigation::parentPath(QStringLiteral("/"), result));

    QVERIFY(DirectoryPathNavigation::parentPath(QStringLiteral("/documents"), result));
    QCOMPARE(result, QStringLiteral("/"));
    QVERIFY(DirectoryPathNavigation::parentPath(QStringLiteral("/documents/reports"), result));
    QCOMPARE(result, QStringLiteral("/documents"));
    QVERIFY(DirectoryPathNavigation::parentPath(QStringLiteral("relative/path"), result));
    QCOMPARE(result, QStringLiteral("relative"));
}

void DirectoryPathNavigationTest::breadcrumbsBuildCumulativePaths()
{
    const QList<DirectoryPathNavigation::Breadcrumb> breadcrumbs =
        DirectoryPathNavigation::breadcrumbs(QStringLiteral("///documents//reports/"));

    QCOMPARE(breadcrumbs.size(), 2);
    QCOMPARE(breadcrumbs.at(0).label, QStringLiteral("documents"));
    QCOMPARE(breadcrumbs.at(0).path, QStringLiteral("/documents"));
    QCOMPARE(breadcrumbs.at(1).label, QStringLiteral("reports"));
    QCOMPARE(breadcrumbs.at(1).path, QStringLiteral("/documents/reports"));
    QVERIFY(DirectoryPathNavigation::breadcrumbs(QStringLiteral("/")).isEmpty());
    QVERIFY(DirectoryPathNavigation::breadcrumbs(QString()).isEmpty());
}

QTEST_APPLESS_MAIN(DirectoryPathNavigationTest)

#include "directory_path_navigation_test.moc"
