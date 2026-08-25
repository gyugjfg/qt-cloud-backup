/**
 * @brief 目录页签纯展示规则的实现。
 *
 * 该文件只负责复用原有字符串规则，不持有页签控件、目录服务或页面状态。
 */
#include "DirectoryTabPresentation.h"

#include <QStringList>

QString DirectoryTabPresentation::title(const QString &nodeName, const QString &path)
{
    QString tabTitle = nodeName;
    if (!path.isEmpty() && path != QStringLiteral("/")) {
        const QString pathName = path.split(QStringLiteral("/")).last();
        if (!pathName.isEmpty()) {
            tabTitle += QStringLiteral(" - ") + pathName;
        }
    }
    return tabTitle;
}

QString DirectoryTabPresentation::nodeName(const QString &tabTitle)
{
    return tabTitle.split(QStringLiteral(" - ")).first();
}
