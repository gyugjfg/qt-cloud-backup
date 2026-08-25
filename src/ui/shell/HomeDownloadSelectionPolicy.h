#ifndef HOMEDOWNLOADSELECTIONPOLICY_H
#define HOMEDOWNLOADSELECTIONPOLICY_H

#include <QList>

/**
 * @brief 主页下载树全选状态的纯判定边界。
 *
 * 该类型只消费树条目的勾选状态，不读取 QWidget、不发送信号，
 * 让 HomeWidge 保留控件编排而把状态规则独立出来。
 */
class HomeDownloadSelectionPolicy
{
public:
    /**
     * @brief 判断条目集合是否应将全选框置为选中。
     *
     * 空集合、部分选中和包含未选中条目时均返回 false；
     * 只有至少一个且全部为 Qt::Checked 时返回 true。
     */
    static bool allItemsChecked(const QList<Qt::CheckState> &states);
};

#endif // HOMEDOWNLOADSELECTIONPOLICY_H
