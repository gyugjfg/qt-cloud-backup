#ifndef HOMEFILEROWPRESENTATION_H
#define HOMEFILEROWPRESENTATION_H

#include "NetworkTypes.h"

#include <QString>

/**
 * @brief HomeWidge 下载树文件行的纯展示数据映射。
 *
 * 该边界只把网络层文件快照转换为树控件所需的文本和 UserRole 值，
 * 不创建 QWidget、不读取窗口状态，也不改变目录或任务服务的语义。
 */
class HomeFileRowPresentation
{
public:
    struct Row
    {
        QString displayName;
        QString sizeText;
        QString modifiedText;
        QString typeText;
        QString filePath;
        bool isDirectory = false;
    };

    /** 将一个网络文件快照转换为主页下载树的一行展示数据。 */
    static Row fromFileInfo(const NetworkFileInfo &fileInfo);

    /** 按主页现有规则格式化文件大小，非正数仍显示“未知”。 */
    static QString formatFileSize(qint64 size);
};

#endif // HOMEFILEROWPRESENTATION_H
