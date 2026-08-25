/*
 * SQLite 持久化实现：封装连接、建表和节点 CRUD。
 * 当前任务运行时仍保存在内存中，不能把本文件的预留 tasks 表描述成恢复能力。
 */
#include "Database.h"
#include <QDebug>

Database::Database(QObject *parent)
    : QObject(parent)
{

}

Database::~Database()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

/**
 * @brief 初始化数据库连接，并确保当前版本需要的表结构存在。
 * @param dbPath 数据库文件路径。
 * @return 初始化是否成功。
 */
bool Database::Initialize(QString dbPath)
{
    // 当前数据库先保证节点表闭环可用；其余表结构先保留给预留设计。
    QSqlDatabase::removeDatabase("backup_db");

    m_db = QSqlDatabase::addDatabase("QSQLITE", "backup_db");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "数据库打开失败:" << m_db.lastError().text();
        return false;
    }

    if (!CreateTables()) {
        return false;
    }

    return true;
}

/**
 * @brief 创建当前版本仍保留的基础表和索引。
 * @return 建表和索引初始化是否成功。
 */
bool Database::CreateTables()
{
    QSqlQuery query(m_db);

    // 当前主用的是节点表，files / tasks 这里只保留结构，不把它们写成已闭环能力。
    if (!m_db.transaction()) {
        qWarning() << "开始事务失败:" << m_db.lastError().text();
        return false;
    }

    QString createNodesTable = "CREATE TABLE IF NOT EXISTS nodes (" 
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, " 
                            "node_id TEXT NOT NULL UNIQUE, " 
                            "name TEXT NOT NULL, " 
                            "ip TEXT NOT NULL, " 
                            "port INTEGER NOT NULL, " 
                            "status INTEGER DEFAULT 0, " 
                            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP" 
                            ");";
    
    if (!query.exec(createNodesTable)) {
        qWarning() << "创建节点表失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    QString createFilesTable = "CREATE TABLE IF NOT EXISTS files (" 
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, " 
                            "name TEXT NOT NULL, " 
                            "path TEXT NOT NULL, " 
                            "size BIGINT NOT NULL, " 
                            "type TEXT, " 
                            "node_id TEXT NOT NULL, " 
                            "user_id TEXT NOT NULL, " 
                            "md5 TEXT, " 
                            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, " 
                            "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP, " 
                            "FOREIGN KEY (node_id) REFERENCES nodes(node_id)" 
                            ");";
    
    if (!query.exec(createFilesTable)) {
        qWarning() << "创建文件表失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    QString createTasksTable = "CREATE TABLE IF NOT EXISTS tasks (" 
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, " 
                            "task_id TEXT NOT NULL UNIQUE, " 
                            "type TEXT NOT NULL, " 
                            "file_id INTEGER, " 
                            "node_id TEXT NOT NULL, " 
                            "status INTEGER DEFAULT 0, " 
                            "progress INTEGER DEFAULT 0, " 
                            "total_size BIGINT DEFAULT 0, " 
                            "transferred_size BIGINT DEFAULT 0, " 
                            "error_message TEXT, " 
                            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, " 
                            "finished_at DATETIME, " 
                            "FOREIGN KEY (file_id) REFERENCES files(id), " 
                            "FOREIGN KEY (node_id) REFERENCES nodes(node_id)" 
                            ");";
    
    if (!query.exec(createTasksTable)) {
        qWarning() << "创建任务表失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    QString createNodeIndex = "CREATE INDEX IF NOT EXISTS idx_nodes_node_id ON nodes(node_id);";
    if (!query.exec(createNodeIndex)) {
        qWarning() << "创建节点索引失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    QString createFileIndex = "CREATE INDEX IF NOT EXISTS idx_files_node_id ON files(node_id);";
    if (!query.exec(createFileIndex)) {
        qWarning() << "创建文件索引失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    QString createTaskIndex = "CREATE INDEX IF NOT EXISTS idx_tasks_task_id ON tasks(task_id);";
    if (!query.exec(createTaskIndex)) {
        qWarning() << "创建任务索引失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    QString createTaskStatusIndex = "CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);";
    if (!query.exec(createTaskStatusIndex)) {
        qWarning() << "创建任务状态索引失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    if (!m_db.commit()) {
        qWarning() << "提交事务失败:" << m_db.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::AddNode(const NetworkNodeInfo &node)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO nodes (node_id, name, ip, port, status) VALUES (:node_id, :name, :ip, :port, :status)");
    query.bindValue(":node_id", node.nodeId);
    query.bindValue(":name", node.nodeName);
    query.bindValue(":ip", node.ip);
    query.bindValue(":port", node.port);
    query.bindValue(":status", node.status);
    
    if (!query.exec()) {
        qWarning() << "添加节点失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::DeleteNode(const QString &nodeId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM nodes WHERE node_id = :node_id");
    query.bindValue(":node_id", nodeId);
    
    if (!query.exec()) {
        qWarning() << "删除节点失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::UpdateNode(const NetworkNodeInfo &node)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE nodes SET name = :name, ip = :ip, port = :port, status = :status WHERE node_id = :node_id");
    query.bindValue(":name", node.nodeName);
    query.bindValue(":ip", node.ip);
    query.bindValue(":port", node.port);
    query.bindValue(":status", node.status);
    query.bindValue(":node_id", node.nodeId);
    
    if (!query.exec()) {
        qWarning() << "更新节点失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QList<NetworkNodeInfo> Database::GetAllNodes()
{
    QList<NetworkNodeInfo> nodes;
    QSqlQuery query(m_db);
    
    if (!query.exec("SELECT node_id, name, ip, port, status FROM nodes")) {
        qWarning() << "获取节点列表失败:" << query.lastError().text();
        return nodes;
    }
    
    while (query.next()) {
        NetworkNodeInfo node;
        node.nodeId = query.value(0).toString();
        node.nodeName = query.value(1).toString();
        node.ip = query.value(2).toString();
        node.port = query.value(3).toInt();
        node.status = query.value(4).toInt();
        nodes.append(node);
    }
    
    return nodes;
}

NetworkNodeInfo Database::GetNode(const QString &nodeId)
{
    NetworkNodeInfo node;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT node_id, name, ip, port, status FROM nodes WHERE node_id = :node_id");
    query.bindValue(":node_id", nodeId);
    
    if (!query.exec()) {
        qWarning() << "获取节点失败:" << query.lastError().text();
        return node;
    }
    
    if (query.next()) {
        node.nodeId = query.value(0).toString();
        node.nodeName = query.value(1).toString();
        node.ip = query.value(2).toString();
        node.port = query.value(3).toInt();
        node.status = query.value(4).toInt();
    }
    
    return node;
}

bool Database::AddFile(const QString &name, const QString &path, qint64 size, const QString &type, const QString &nodeId, const QString &userId)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO files (name, path, size, type, node_id, user_id) VALUES (:name, :path, :size, :type, :node_id, :user_id)");
    query.bindValue(":name", name);
    query.bindValue(":path", path);
    query.bindValue(":size", size);
    query.bindValue(":type", type);
    query.bindValue(":node_id", nodeId);
    query.bindValue(":user_id", userId);
    
    if (!query.exec()) {
        qWarning() << "添加文件失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::DeleteFile(int fileId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM files WHERE id = :id");
    query.bindValue(":id", fileId);
    
    if (!query.exec()) {
        qWarning() << "删除文件失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::UpdateFile(int fileId, const QString &name, const QString &path, qint64 size, const QString &type)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE files SET name = :name, path = :path, size = :size, type = :type, updated_at = CURRENT_TIMESTAMP WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":path", path);
    query.bindValue(":size", size);
    query.bindValue(":type", type);
    query.bindValue(":id", fileId);
    
    if (!query.exec()) {
        qWarning() << "更新文件失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QList<QMap<QString, QVariant>> Database::GetFilesByNode(const QString &nodeId)
{
    QList<QMap<QString, QVariant>> files;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT id, name, path, size, type, md5, created_at FROM files WHERE node_id = :node_id");
    query.bindValue(":node_id", nodeId);
    
    if (!query.exec()) {
        qWarning() << "获取文件列表失败:" << query.lastError().text();
        return files;
    }
    
    while (query.next()) {
        QMap<QString, QVariant> file;
        file["id"] = query.value(0).toInt();
        file["name"] = query.value(1).toString();
        file["path"] = query.value(2).toString();
        file["size"] = query.value(3).toLongLong();
        file["type"] = query.value(4).toString();
        file["md5"] = query.value(5).toString();
        file["created_at"] = query.value(6).toString();
        files.append(file);
    }
    
    return files;
}

bool Database::AddTask(const QString &taskId, const QString &type, int fileId, const QString &nodeId, int status, int progress, qint64 totalSize, qint64 transferredSize)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO tasks (task_id, type, file_id, node_id, status, progress, total_size, transferred_size) VALUES (:task_id, :type, :file_id, :node_id, :status, :progress, :total_size, :transferred_size)");
    query.bindValue(":task_id", taskId);
    query.bindValue(":type", type);
    query.bindValue(":file_id", fileId);
    query.bindValue(":node_id", nodeId);
    query.bindValue(":status", status);
    query.bindValue(":progress", progress);
    query.bindValue(":total_size", totalSize);
    query.bindValue(":transferred_size", transferredSize);
    
    if (!query.exec()) {
        qWarning() << "添加任务失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::UpdateTask(const QString &taskId, int status, int progress, qint64 transferredSize)
{
    QSqlQuery query(m_db);
    
    QString sql = "UPDATE tasks SET status = :status, progress = :progress, transferred_size = :transferred_size";
    if (status == 3) {
        sql += ", finished_at = CURRENT_TIMESTAMP";
    }
    sql += " WHERE task_id = :task_id";
    
    query.prepare(sql);
    query.bindValue(":status", status);
    query.bindValue(":progress", progress);
    query.bindValue(":transferred_size", transferredSize);
    query.bindValue(":task_id", taskId);
    
    if (!query.exec()) {
        qWarning() << "更新任务失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::DeleteTask(const QString &taskId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM tasks WHERE task_id = :task_id");
    query.bindValue(":task_id", taskId);
    
    if (!query.exec()) {
        qWarning() << "删除任务失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QList<QMap<QString, QVariant>> Database::GetAllTasks()
{
    QList<QMap<QString, QVariant>> tasks;
    QSqlQuery query(m_db);
    
    if (!query.exec("SELECT task_id, type, file_id, node_id, status, progress, total_size, transferred_size, error_message, created_at, finished_at FROM tasks")) {
        qWarning() << "获取任务列表失败:" << query.lastError().text();
        return tasks;
    }
    
    while (query.next()) {
        QMap<QString, QVariant> task;
        task["task_id"] = query.value(0).toString();
        task["type"] = query.value(1).toString();
        task["file_id"] = query.value(2).toInt();
        task["node_id"] = query.value(3).toString();
        task["status"] = query.value(4).toInt();
        task["progress"] = query.value(5).toInt();
        task["total_size"] = query.value(6).toLongLong();
        task["transferred_size"] = query.value(7).toLongLong();
        task["error_message"] = query.value(8).toString();
        task["created_at"] = query.value(9).toString();
        task["finished_at"] = query.value(10).toString();
        tasks.append(task);
    }
    
    return tasks;
}

QMap<QString, QVariant> Database::GetTask(const QString &taskId)
{
    QMap<QString, QVariant> task;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT task_id, type, file_id, node_id, status, progress, total_size, transferred_size, error_message, created_at, finished_at FROM tasks WHERE task_id = :task_id");
    query.bindValue(":task_id", taskId);
    
    if (!query.exec()) {
        qWarning() << "获取任务失败:" << query.lastError().text();
        return task;
    }
    
    if (query.next()) {
        task["task_id"] = query.value(0).toString();
        task["type"] = query.value(1).toString();
        task["file_id"] = query.value(2).toInt();
        task["node_id"] = query.value(3).toString();
        task["status"] = query.value(4).toInt();
        task["progress"] = query.value(5).toInt();
        task["total_size"] = query.value(6).toLongLong();
        task["transferred_size"] = query.value(7).toLongLong();
        task["error_message"] = query.value(8).toString();
        task["created_at"] = query.value(9).toString();
        task["finished_at"] = query.value(10).toString();
    }
    
    return task;
}
