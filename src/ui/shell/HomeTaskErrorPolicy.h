#ifndef HOMETASKERRORPOLICY_H
#define HOMETASKERRORPOLICY_H

#include <QString>

/**
 * @brief 主页任务错误反馈的纯标题与去重键规则。
 *
 * 该边界只根据任务 ID 和错误文本生成展示所需字符串，不创建弹窗、
 * 不维护去重集合，也不依赖网络或任务对象的生命周期。
 */
class HomeTaskErrorPolicy
{
public:
    /** 根据任务 ID 前缀返回原有上传/下载错误标题。 */
    static QString titleForTask(const QString &taskId);

    /** 使用原有标题和错误文本组合主页级去重键。 */
    static QString deduplicationKey(const QString &taskId,
                                    const QString &errorMessage);
};

#endif // HOMETASKERRORPOLICY_H
