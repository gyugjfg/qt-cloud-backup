/**
 * @file NodeModule.cpp
 * @brief 节点列表、选择器和节点数据同步的业务实现。
 *
 * 调用链保持为 UI -> NodeModule -> Database/NodeGateway；模块只在 GUI
 * 线程操作 Qt 控件，注入对象的生命周期由主页组合根负责。
 */
#include "NodeModule.h"

#include "Database.h"
#include "NodeGateway.h"
#include "NodeDialog.h"
#include "NodeItem.h"

#include <QCheckBox>
#include <QDateTime>
#include <QMessageBox>
#include <QSignalBlocker>

NodeModule::NodeModule(Database *db, NodeGateway *nodeGateway, QObject *parent)
    : QObject(parent)
    , m_db(db)
    , m_nodeGateway(nodeGateway)
{
}

void NodeModule::setUiComponents(QListWidget *nodeListWidget,
                                 QComboBox *downloadNodeCombo,
                                 QComboBox *uploadNodeCombo)
{
    m_nodeListWidget = nodeListWidget;
    m_downloadNodeCombo = downloadNodeCombo;
    m_uploadNodeCombo = uploadNodeCombo;
}

/**
 * @brief 从数据库加载节点，并同步到列表和节点选择器。
 */
void NodeModule::loadNodesFromDatabase()
{
    if (!m_db || !m_nodeGateway || !m_nodeListWidget || !m_downloadNodeCombo || !m_uploadNodeCombo) {
        return;
    }

    m_nodeListWidget->clear();
    m_downloadNodeCombo->clear();
    m_uploadNodeCombo->clear();
    m_checkNodeItems.clear();

    m_downloadNodeCombo->addItem(QStringLiteral("选择节点"));
    m_uploadNodeCombo->addItem(QStringLiteral("选择节点"));

    const QList<NetworkNodeInfo> nodes = m_db->GetAllNodes();
    for (const NetworkNodeInfo &node : nodes) {
        appendNodeListItem(node);
        m_nodeGateway->addNode(node);
        addNodeToSelectors(node);
    }

    m_downloadNodeCombo->setCurrentIndex(0);
    // 节点数据加载完成后再通知页面层，避免监听方读取到半成品列表。
    emit nodesLoaded();
}

void NodeModule::applyNodeDataToItem(NodeItem *nodeItem, const NetworkNodeInfo &node)
{
    if (!nodeItem) {
        return;
    }

    nodeItem->setNodeName(node.nodeName);
    nodeItem->setNodeIP(node.ip);
    nodeItem->setNodePort(QString::number(node.port));
}

void NodeModule::addNodeToSelectors(const NetworkNodeInfo &node)
{
    if (m_downloadNodeCombo) {
        m_downloadNodeCombo->addItem(node.nodeName, node.nodeId);
    }
    if (m_uploadNodeCombo) {
        m_uploadNodeCombo->addItem(node.nodeName, node.nodeId);
    }
}

void NodeModule::syncNodeSelectors(const QString &nodeId, const QString &nodeName)
{
    if (m_downloadNodeCombo) {
        const int downloadIndex = m_downloadNodeCombo->findData(nodeId);
        if (downloadIndex != -1) {
            m_downloadNodeCombo->setItemText(downloadIndex, nodeName);
        }
    }

    if (m_uploadNodeCombo) {
        const int uploadIndex = m_uploadNodeCombo->findData(nodeId);
        if (uploadIndex != -1) {
            m_uploadNodeCombo->setItemText(uploadIndex, nodeName);
        }
    }
}

void NodeModule::removeNodeFromSelectors(const QString &nodeId)
{
    if (m_downloadNodeCombo) {
        const int downloadIndex = m_downloadNodeCombo->findData(nodeId);
        if (downloadIndex != -1) {
            m_downloadNodeCombo->removeItem(downloadIndex);
        }
    }

    if (m_uploadNodeCombo) {
        const int uploadIndex = m_uploadNodeCombo->findData(nodeId);
        if (uploadIndex != -1) {
            m_uploadNodeCombo->removeItem(uploadIndex);
        }
    }
}

QListWidgetItem *NodeModule::findNodeListItem(const NodeItem *nodeItem) const
{
    if (!m_nodeListWidget || !nodeItem) {
        return nullptr;
    }

    for (int i = 0; i < m_nodeListWidget->count(); ++i) {
        QListWidgetItem *item = m_nodeListWidget->item(i);
        if (item && m_nodeListWidget->itemWidget(item) == nodeItem) {
            return item;
        }
    }

    return nullptr;
}

QListWidgetItem *NodeModule::selectedSingleNodeListItem() const
{
    if (m_checkNodeItems.size() != 1) {
        return nullptr;
    }

    return findNodeListItem(qobject_cast<NodeItem*>(m_checkNodeItems.values().first()));
}

void NodeModule::refreshNodeItemIndices()
{
    if (!m_nodeListWidget) {
        return;
    }

    m_checkNodeItems.clear();
    for (int i = 0; i < m_nodeListWidget->count(); ++i) {
        QListWidgetItem *item = m_nodeListWidget->item(i);
        NodeItem *nodeItem = item ? qobject_cast<NodeItem*>(m_nodeListWidget->itemWidget(item)) : nullptr;
        if (!nodeItem) {
            continue;
        }

        nodeItem->setItemIndex(i);
        if (nodeItem->isChecked()) {
            m_checkNodeItems[i] = nodeItem;
        }
    }
}

void NodeModule::appendNodeListItem(const NetworkNodeInfo &node, int itemIndex)
{
    if (!m_nodeListWidget) {
        return;
    }

    QListWidgetItem *item = new QListWidgetItem(m_nodeListWidget);
    NodeItem *nodeItem = new NodeItem(m_nodeListWidget);
    item->setSizeHint(nodeItem->size());

    applyNodeDataToItem(nodeItem, node);
    if (itemIndex >= 0) {
        nodeItem->setItemIndex(itemIndex);
    }

    connect(nodeItem, &NodeItem::checkStatusChanged, this, [this](bool status, QWidget *widget) {
        NodeItem *itemWidget = qobject_cast<NodeItem*>(widget);
        if (!itemWidget) {
            return;
        }

        if (status) {
            m_checkNodeItems[itemWidget->getItemIndex()] = itemWidget;
        } else {
            m_checkNodeItems.remove(itemWidget->getItemIndex());
        }

        emit checkedNodeItemsChanged(m_checkNodeItems.size(),
                                     m_nodeListWidget ? m_nodeListWidget->count() : 0);
    });

    m_nodeListWidget->addItem(item);
    m_nodeListWidget->setItemWidget(item, nodeItem);
    item->setData(Qt::UserRole, node.nodeId);
    refreshNodeItemIndices();
    emit checkedNodeItemsChanged(m_checkNodeItems.size(),
                                 m_nodeListWidget ? m_nodeListWidget->count() : 0);
}

void NodeModule::setAllNodesChecked(bool checked)
{
    if (!m_nodeListWidget) {
        return;
    }

    m_checkNodeItems.clear();
    for (int i = 0; i < m_nodeListWidget->count(); ++i) {
        QListWidgetItem *item = m_nodeListWidget->item(i);
        NodeItem *nodeItem = item ? qobject_cast<NodeItem*>(m_nodeListWidget->itemWidget(item)) : nullptr;
        if (!nodeItem) {
            continue;
        }

        const QSignalBlocker blocker(nodeItem);
        nodeItem->setCheckStatus(checked ? Qt::Checked : Qt::Unchecked);
        if (checked) {
            m_checkNodeItems[i] = nodeItem;
        }
    }

    emit checkedNodeItemsChanged(m_checkNodeItems.size(), m_nodeListWidget->count());
}

/**
 * @brief 创建一条新的节点配置，并同步到数据库、列表和选择器。
 * @param dialogParent 节点编辑对话框父对象。
 * @param createdNode 返回新建成功后的节点信息。
 * @return 节点是否创建成功。
 */
bool NodeModule::createNode(QWidget *dialogParent, NetworkNodeInfo &createdNode)
{
    if (!m_db || !m_nodeGateway) {
        return false;
    }

    NodeDialog nodeDialog(dialogParent);
    nodeDialog.setTitle(QString::fromUtf8(u8"新增节点"));
    if (nodeDialog.exec() != QDialog::Accepted) {
        return false;
    }

    createdNode.nodeId = "node_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    createdNode.nodeName = nodeDialog.getNodeName();
    createdNode.ip = nodeDialog.getNodeIP();
    createdNode.port = nodeDialog.getNodePort().toInt();
    createdNode.status = 0;

    m_nodeGateway->addNode(createdNode);
    m_db->AddNode(createdNode);
    appendNodeListItem(createdNode);
    addNodeToSelectors(createdNode);
    return true;
}

/**
 * @brief 删除当前勾选节点，并同步列表、选择器和数据库。
 * @param currentNodeId 当前下载页正在使用的节点 ID。
 * @param removedNodeIds 返回本次实际删除的节点 ID 集合。
 * @return 当前激活节点是否被本次删除。
 */
bool NodeModule::removeSelectedNodes(const QString &currentNodeId, QList<QString> &removedNodeIds)
{
    if (!m_db || !m_nodeGateway || !m_nodeListWidget) {
        return false;
    }

    if (m_checkNodeItems.isEmpty()) {
        return false;
    }

    removedNodeIds.clear();
    for (QWidget *widget : m_checkNodeItems.values()) {
        QListWidgetItem *item = findNodeListItem(qobject_cast<NodeItem*>(widget));
        if (item) {
            removedNodeIds.append(item->data(Qt::UserRole).toString());
        }
    }

    if (removedNodeIds.isEmpty()) {
        return false;
    }

    for (int i = m_nodeListWidget->count() - 1; i >= 0; --i) {
        QListWidgetItem *item = m_nodeListWidget->item(i);
        if (!item) {
            continue;
        }

        const QString nodeId = item->data(Qt::UserRole).toString();
        if (!removedNodeIds.contains(nodeId)) {
            continue;
        }

        m_nodeGateway->removeNode(nodeId);
        m_db->DeleteNode(nodeId);
        delete m_nodeListWidget->takeItem(i);
        removeNodeFromSelectors(nodeId);
    }

    refreshNodeItemIndices();
    emit checkedNodeItemsChanged(m_checkNodeItems.size(), m_nodeListWidget->count());
    return removedNodeIds.contains(currentNodeId);
}

/**
 * @brief 修改当前选中节点，并同步数据库、列表和选择器。
 * @param dialogParent 节点编辑对话框父对象。
 * @param updatedNode 返回更新后的节点信息。
 * @return 节点是否更新成功。
 */
bool NodeModule::updateSelectedNode(QWidget *dialogParent, NetworkNodeInfo &updatedNode)
{
    if (!m_db || !m_nodeGateway || !m_nodeListWidget) {
        return false;
    }

    QListWidgetItem *selectedItem = selectedSingleNodeListItem();
    if (!selectedItem) {
        return false;
    }

    const QString nodeId = selectedItem->data(Qt::UserRole).toString();
    const NetworkNodeInfo node = m_nodeGateway->nodeInfo(nodeId);
    NodeItem *nodeItem = qobject_cast<NodeItem*>(m_nodeListWidget->itemWidget(selectedItem));
    if (!nodeItem) {
        return false;
    }

    NodeDialog nodeDialog(dialogParent);
    nodeDialog.setTitle(QString::fromUtf8(u8"修改节点配置"));
    nodeDialog.setNodeInfo(node.nodeName, node.ip, QString::number(node.port));
    if (nodeDialog.exec() != QDialog::Accepted) {
        return false;
    }

    updatedNode.nodeId = nodeId;
    updatedNode.nodeName = nodeDialog.getNodeName();
    updatedNode.ip = nodeDialog.getNodeIP();
    updatedNode.port = nodeDialog.getNodePort().toInt();
    updatedNode.status = 0;

    m_nodeGateway->updateNode(updatedNode);
    m_db->UpdateNode(updatedNode);
    applyNodeDataToItem(nodeItem, updatedNode);
    syncNodeSelectors(nodeId, updatedNode.nodeName);
    return true;
}
