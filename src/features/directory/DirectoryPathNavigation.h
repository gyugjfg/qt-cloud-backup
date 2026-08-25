#ifndef DIRECTORYPATHNAVIGATION_H
#define DIRECTORYPATHNAVIGATION_H

#include <QList>
#include <QString>

// 目录路径的纯转换边界：只计算父路径和面包屑目标，不持有页面或网络状态。
class DirectoryPathNavigation
{
public:
    struct Breadcrumb {
        QString label;
        QString path;
    };

    /**
     * @brief 按目录模块现有规则计算当前路径的父路径。
     * @param currentPath 当前目录路径。
     * @param result 输出父路径。
     * @return 当前路径存在可返回的上级目录时为 true。
     */
    static bool parentPath(const QString &currentPath, QString &result);

    /**
     * @brief 按路径段生成面包屑按钮所需的标签和累积路径。
     * @param path 当前目录路径。
     * @return 按原有显示顺序排列的面包屑段。
     */
    static QList<Breadcrumb> breadcrumbs(const QString &path);
};

#endif // DIRECTORYPATHNAVIGATION_H
