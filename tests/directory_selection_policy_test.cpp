#include <QtTest>

#include "DirectorySelectionPolicy.h"

class DirectorySelectionPolicyTest final : public QObject
{
    Q_OBJECT

private slots:
    void acceptedResultWithNodeAndPathIsValid();
    void rejectedOrIncompleteResultIsInvalid();
};

void DirectorySelectionPolicyTest::acceptedResultWithNodeAndPathIsValid()
{
    QVERIFY(DirectorySelectionPolicy::isValidResult(
        true, 0, QStringLiteral("/documents")));
    QVERIFY(DirectorySelectionPolicy::isValidResult(
        true, 3, QStringLiteral("relative/path")));
}

void DirectorySelectionPolicyTest::rejectedOrIncompleteResultIsInvalid()
{
    QVERIFY(!DirectorySelectionPolicy::isValidResult(
        false, 0, QStringLiteral("/documents")));
    QVERIFY(!DirectorySelectionPolicy::isValidResult(
        true, -1, QStringLiteral("/documents")));
    QVERIFY(!DirectorySelectionPolicy::isValidResult(true, 0, QString()));
}

QTEST_APPLESS_MAIN(DirectorySelectionPolicyTest)

#include "directory_selection_policy_test.moc"
