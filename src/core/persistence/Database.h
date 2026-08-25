#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QObject>
#include "NetworkTypes.h"

// 当前数据库真实主用闭环是节点表；files / tasks 更偏预留设计，任务主线不依赖这里做完整持久化。
class Database : public QObject
{
    Q_OBJECT

public:
    Database(QObject *parent = nullptr);
    ~Database();

    /**
     * @brief 初始化数据库连接并确保当前表结构存在。
     * @param dbPath 数据库文件路径。
     * @return 初始化是否成功。
     */
    bool Initialize(QString dbPath);

    bool AddNode(const NetworkNodeInfo &node);
    bool DeleteNode(const QString &nodeId);
    bool UpdateNode(const NetworkNodeInfo &node);
    QList<NetworkNodeInfo> GetAllNodes();
    NetworkNodeInfo GetNode(const QString &nodeId);

    // 文件 / 任务表接口暂时保留，但当前主线没有把它们接成完整业务闭环。
    bool AddFile(const QString &name, const QString &path, qint64 size, const QString &type, const QString &nodeId, const QString &userId);
    bool DeleteFile(int fileId);
    bool UpdateFile(int fileId, const QString &name, const QString &path, qint64 size, const QString &type);
    QList<QMap<QString, QVariant>> GetFilesByNode(const QString &nodeId);

    bool AddTask(const QString &taskId, const QString &type, int fileId, const QString &nodeId, int status, int progress, qint64 totalSize, qint64 transferredSize);
    bool UpdateTask(const QString &taskId, int status, int progress, qint64 transferredSize);
    bool DeleteTask(const QString &taskId);
    QList<QMap<QString, QVariant>> GetAllTasks();
    QMap<QString, QVariant> GetTask(const QString &taskId);

private:
    QSqlDatabase m_db;

    /**
     * @brief 创建当前版本仍保留的基础表结构。
     * @return 建表和索引初始化是否成功。
     */
    bool CreateTables();
};

#endif // DATABASE_H
