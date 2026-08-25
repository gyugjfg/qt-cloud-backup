/**
 * @brief 目录路径导航纯规则的实现。
 *
 * 该文件只保留目录模块原有的字符串规则，不执行路径规范化或权限判断。
 */
#include "DirectoryPathNavigation.h"

#include <QStringList>

bool DirectoryPathNavigation::parentPath(const QString &currentPath, QString &result)
{
    if (currentPath.isEmpty() || currentPath == QStringLiteral("/")) {
        return false;
    }

    const int lastSlashIndex = currentPath.lastIndexOf(QStringLiteral("/"));
    result = lastSlashIndex > 0 ? currentPath.left(lastSlashIndex) : QStringLiteral("/");
    return true;
}

QList<DirectoryPathNavigation::Breadcrumb>
DirectoryPathNavigation::breadcrumbs(const QString &path)
{
    QList<Breadcrumb> result;
    QString currentPath;
    const QStringList pathParts = path.split(QStringLiteral("/"), Qt::SkipEmptyParts);
    for (const QString &part : pathParts) {
        currentPath += QStringLiteral("/") + part;
        result.append({part, currentPath});
    }
    return result;
}
