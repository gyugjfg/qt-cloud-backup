#include "NodeGateway.h"

#include <QSet>
#include <QStringList>
#include <QtTest>

class FakeNodeGateway final : public NodeGateway
{
public:
    FakeNodeGateway()
        : NodeGateway(nullptr)
    {
    }

    bool addNode(const NodeInfo &node) override
    {
        addedNodes.append(node);
        return true;
    }

    bool removeNode(const QString &nodeId) override
    {
        removedNodeIds.append(nodeId);
        return true;
    }

    bool updateNode(const NodeInfo &node) override
    {
        updatedNodes.append(node);
        return true;
    }

    QList<NodeInfo> nodeList() const override
    {
        return nodes;
    }

    NodeInfo nodeInfo(const QString &nodeId) const override
    {
        for (const NodeInfo &node : nodes) {
            if (node.nodeId == nodeId) {
                return node;
            }
        }
        return NodeInfo();
    }

    bool checkNodeStatus(const QString &nodeId) override
    {
        checkedNodeIds.append(nodeId);
        return onlineNodeIds.contains(nodeId);
    }

    QList<NodeInfo> nodes;
    QList<NodeInfo> addedNodes;
    QList<NodeInfo> updatedNodes;
    QStringList removedNodeIds;
    QStringList checkedNodeIds;
    QSet<QString> onlineNodeIds;
};

class NodeGatewayContractTest : public QObject
{
    Q_OBJECT

private slots:
    /** 空网络依赖必须保持原有失败和默认值语义。 */
    void nullNetworkKeepsFailureSemantics();
    /** 节点值对象和调用参数必须能通过可替换 Gateway 端口观察。 */
    void replacementGatewayReceivesNodeOperations();
};

void NodeGatewayContractTest::nullNetworkKeepsFailureSemantics()
{
    NodeGateway gateway(nullptr);
    NodeGateway::NodeInfo node;
    node.nodeId = QStringLiteral("node-1");
    node.nodeName = QStringLiteral("测试节点");

    QVERIFY(!gateway.addNode(node));
    QVERIFY(!gateway.removeNode(node.nodeId));
    QVERIFY(!gateway.updateNode(node));
    QVERIFY(gateway.nodeList().isEmpty());
    QVERIFY(gateway.nodeInfo(node.nodeId).nodeId.isEmpty());
    QVERIFY(!gateway.checkNodeStatus(node.nodeId));
}

void NodeGatewayContractTest::replacementGatewayReceivesNodeOperations()
{
    FakeNodeGateway fakeGateway;
    NodeGateway *gateway = &fakeGateway;

    NodeGateway::NodeInfo node;
    node.nodeId = QStringLiteral("node-7");
    node.nodeName = QStringLiteral("测试节点");
    node.ip = QStringLiteral("127.0.0.1");
    node.port = 9000;

    QVERIFY(gateway->addNode(node));
    QCOMPARE(fakeGateway.addedNodes.size(), 1);
    QCOMPARE(fakeGateway.addedNodes.constFirst().nodeId, node.nodeId);

    fakeGateway.nodes.append(node);
    const QList<NodeGateway::NodeInfo> nodes = gateway->nodeList();
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes.constFirst().nodeName, node.nodeName);
    QCOMPARE(gateway->nodeInfo(node.nodeId).ip, node.ip);

    node.nodeName = QStringLiteral("更新后的节点");
    QVERIFY(gateway->updateNode(node));
    QCOMPARE(fakeGateway.updatedNodes.size(), 1);
    QCOMPARE(fakeGateway.updatedNodes.constFirst().nodeName, node.nodeName);

    QVERIFY(gateway->removeNode(node.nodeId));
    QCOMPARE(fakeGateway.removedNodeIds, QStringList{node.nodeId});

    fakeGateway.onlineNodeIds.insert(node.nodeId);
    QVERIFY(gateway->checkNodeStatus(node.nodeId));
    QCOMPARE(fakeGateway.checkedNodeIds, QStringList{node.nodeId});
}

QTEST_MAIN(NodeGatewayContractTest)
#include "node_gateway_contract_test.moc"
