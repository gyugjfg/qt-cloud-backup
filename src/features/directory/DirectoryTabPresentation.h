#ifndef DIRECTORYTABPRESENTATION_H
#define DIRECTORYTABPRESENTATION_H

#include <QString>

// 目录页签的纯展示规则：统一路径标题和节点名称的字符串转换，不持有 UI 状态。
class DirectoryTabPresentation
{
public:
    /**
     * @brief 根据节点名称和目录路径生成页签标题。
     * @param nodeName 节点显示名称。
     * @param path 当前目录路径。
     * @return 与原页签规则一致的展示文本。
     */
    static QString title(const QString &nodeName, const QString &path);

    /**
     * @brief 从页签标题中恢复节点名称。
     * @param tabTitle 现有页签标题。
     * @return 标题中路径后缀前的节点名称。
     */
    static QString nodeName(const QString &tabTitle);
};

#endif // DIRECTORYTABPRESENTATION_H
