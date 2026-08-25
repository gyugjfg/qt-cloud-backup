#include "HomeDownloadSelectionPolicy.h"

#include <QtTest>

class HomeDownloadSelectionPolicyTest : public QObject
{
    Q_OBJECT

private slots:
    /** 空集合不应把主页全选框误判为选中。 */
    void emptyStatesAreNotChecked();
    /** 只有至少一个且全部选中时才返回 true。 */
    void allCheckedStatesAreChecked();
    /** 未选中和半选状态都应阻止全选框进入选中态。 */
    void mixedStatesAreNotChecked();
};

void HomeDownloadSelectionPolicyTest::emptyStatesAreNotChecked()
{
    QVERIFY(!HomeDownloadSelectionPolicy::allItemsChecked({}));
}

void HomeDownloadSelectionPolicyTest::allCheckedStatesAreChecked()
{
    QVERIFY(HomeDownloadSelectionPolicy::allItemsChecked({Qt::Checked}));
    QVERIFY(HomeDownloadSelectionPolicy::allItemsChecked({Qt::Checked, Qt::Checked}));
}

void HomeDownloadSelectionPolicyTest::mixedStatesAreNotChecked()
{
    QVERIFY(!HomeDownloadSelectionPolicy::allItemsChecked({Qt::Unchecked}));
    QVERIFY(!HomeDownloadSelectionPolicy::allItemsChecked({Qt::Checked, Qt::Unchecked}));
    QVERIFY(!HomeDownloadSelectionPolicy::allItemsChecked({Qt::PartiallyChecked}));
}

QTEST_MAIN(HomeDownloadSelectionPolicyTest)
#include "home_download_selection_policy_test.moc"
