/**
 * @brief 主页下载树全选状态纯判定的实现。
 */
#include "HomeDownloadSelectionPolicy.h"

bool HomeDownloadSelectionPolicy::allItemsChecked(const QList<Qt::CheckState> &states)
{
    if (states.isEmpty()) {
        return false;
    }

    for (const Qt::CheckState state : states) {
        if (state != Qt::Checked) {
            return false;
        }
    }

    return true;
}
