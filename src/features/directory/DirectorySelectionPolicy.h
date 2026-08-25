#ifndef DIRECTORYSELECTIONPOLICY_H
#define DIRECTORYSELECTIONPOLICY_H

#include <QString>

/**
 * @brief 目录选择对话框结果的无状态校验规则。
 *
 * 该边界只消费对话框返回的值，不读取 QWidget、QObject 或 Gateway。
 * DirectoryPageController 继续负责对话框编排和结果回写，策略只负责回答
 * “当前结果是否可以交给后续目录页流程”的业务问题。
 */
namespace DirectorySelectionPolicy {

/**
 * @brief 判断目录选择结果是否满足原有提交前置条件。
 * @param accepted 对话框是否以 Accepted 结果结束。
 * @param nodeIndex 结果对应的节点下拉索引。
 * @param path 用户选中的目录路径。
 * @return 对话框已确认、节点索引有效且路径非空时返回 true。
 */
inline bool isValidResult(bool accepted, int nodeIndex, const QString &path)
{
    return accepted && nodeIndex >= 0 && !path.isEmpty();
}

} // namespace DirectorySelectionPolicy

#endif // DIRECTORYSELECTIONPOLICY_H
