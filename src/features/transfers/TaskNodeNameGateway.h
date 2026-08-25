#ifndef TASKNODENAMEGATEWAY_H
#define TASKNODENAMEGATEWAY_H

#include <QString>
#include <QObject>

class NodeGateway;

/**
 * @brief 任务页使用的节点名称查询窄端口。
 *
 * 该对象只读 NodeGateway 的节点信息并返回展示名称，不缓存结果、不修改
 * 节点状态。查询失败或 Gateway 未注入时返回空字符串。
 */
class TaskNodeNameGateway final : public QObject
{
    Q_OBJECT

public:
    /** 注入节点查询端口；指针由组合根拥有。 */
    explicit TaskNodeNameGateway(NodeGateway *nodeGateway, QObject *parent = nullptr);

    /** 按节点 ID 返回页面展示名称；节点不存在时返回空字符串。 */
    QString nodeName(const QString &nodeId) const;

private:
    NodeGateway *m_nodeGateway; ///< 借用的节点 Gateway。
};

#endif // TASKNODENAMEGATEWAY_H
