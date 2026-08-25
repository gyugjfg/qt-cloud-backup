/**
 * @brief 主页任务错误反馈纯字符串规则的实现。
 */
#include "HomeTaskErrorPolicy.h"

QString HomeTaskErrorPolicy::titleForTask(const QString &taskId)
{
    return taskId.startsWith(QStringLiteral("upload_"))
        ? QStringLiteral("上传错误")
        : QStringLiteral("下载错误");
}

QString HomeTaskErrorPolicy::deduplicationKey(const QString &taskId,
                                               const QString &errorMessage)
{
    return titleForTask(taskId) + QStringLiteral("|") + errorMessage;
}
