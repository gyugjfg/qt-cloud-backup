#ifndef TASKPRESENTATIONPOLICY_H
#define TASKPRESENTATIONPOLICY_H

#include "TransferTypes.h"

#include <QString>
#include <QtGlobal>

/**
 * @brief 任务页使用的纯展示规则。
 *
 * 该策略只把稳定的状态值和数值转换成页面文案，不读取任务仓库、文件系统
 * 或 QWidget。TaskModule 负责取得快照和编排列表，策略负责最后的字符串规则。
 */
class TaskPresentationPolicy
{
public:
    /** 根据运行态和传输方向生成活动任务状态文案。 */
    static QString taskStatusText(int status, bool isDownload)
    {
        switch (status) {
        case static_cast<int>(NetworkTransferStatus::Waiting):
            return waitingText();
        case static_cast<int>(NetworkTransferStatus::Running):
            return isDownload ? downloadingText() : uploadingText();
        case static_cast<int>(NetworkTransferStatus::Paused):
            return pausedText();
        case static_cast<int>(NetworkTransferStatus::Completed):
            return isDownload ? downloadedText() : uploadedText();
        case static_cast<int>(NetworkTransferStatus::Failed):
            return failedText();
        case static_cast<int>(NetworkTransferStatus::Canceled):
            return canceledText();
        default:
            return unknownText();
        }
    }

    /** 根据终态和传输方向生成已完成列表状态文案。 */
    static QString finishedStatusText(int status, bool isDownload)
    {
        if (status == static_cast<int>(NetworkTransferStatus::Completed)) {
            return isDownload ? downloadedText() : uploadedText();
        }
        if (status == static_cast<int>(NetworkTransferStatus::Paused)) {
            return pausedText();
        }
        if (status == static_cast<int>(NetworkTransferStatus::Canceled)) {
            return canceledText();
        }
        return failedText();
    }

    /** 将字节数格式化为任务列表使用的两位小数单位文案。 */
    static QString formatFileSize(qint64 size)
    {
        if (size <= 0) {
            return unknownText();
        }
        if (size < 1024) {
            return QString::number(size) + QStringLiteral(" B");
        }
        if (size < 1024 * 1024) {
            return QString::number(size / 1024.0, 'f', 2) + QStringLiteral(" KB");
        }
        if (size < 1024 * 1024 * 1024) {
            return QString::number(size / (1024.0 * 1024), 'f', 2) + QStringLiteral(" MB");
        }
        return QString::number(size / (1024.0 * 1024 * 1024), 'f', 2) + QStringLiteral(" GB");
    }

    /** 返回任务摘要中的上传或下载类型文案。 */
    static QString taskTypeText(bool isDownload)
    {
        return isDownload ? downloadText() : uploadText();
    }

    /**
     * @brief 从旧任务行的速度/大小混合文本中提取大小部分。
     *
     * 终态失败、取消、未知和速度文本仍返回空字符串，交由 TaskModule 的
     * 原有终态兜底逻辑决定最终文案。
     */
    static QString sizeFromProgressDisplay(const QString &displayText)
    {
        QString text = displayText.trimmed();
        if (text.isEmpty() || text == QStringLiteral("--")
            || text == failedText() || text == canceledText() || text == unknownText()) {
            return QString();
        }

        if (text.contains(QStringLiteral("|"))) {
            text = text.section(QStringLiteral("|"), -1).trimmed();
        }
        if (text.contains(QStringLiteral("/"))) {
            text = text.section(QStringLiteral("/"), -1).trimmed();
        }

        if (text.isEmpty() || text == QStringLiteral("--")
            || text == failedText() || text == canceledText() || text == unknownText()
            || text.contains(QStringLiteral("/s"))) {
            return QString();
        }
        return text;
    }

    /** 失败终态使用的固定文案，供 TaskModule 兜底。 */
    static QString failedText()
    {
        return QString::fromUtf8(u8"\u5931\u8d25");
    }

    /** 取消终态使用的固定文案，供 TaskModule 兜底。 */
    static QString canceledText()
    {
        return QString::fromUtf8(u8"\u5df2\u53d6\u6d88");
    }

    /** 未知状态或未知大小使用的固定文案。 */
    static QString unknownText()
    {
        return QString::fromUtf8(u8"\u672a\u77e5");
    }

private:
    static QString waitingText()
    {
        return QString::fromUtf8(u8"\u7b49\u5f85\u4e2d");
    }

    static QString downloadingText()
    {
        return QString::fromUtf8(u8"\u4e0b\u8f7d\u4e2d");
    }

    static QString uploadingText()
    {
        return QString::fromUtf8(u8"\u4e0a\u4f20\u4e2d");
    }

    static QString pausedText()
    {
        return QString::fromUtf8(u8"\u5df2\u6682\u505c");
    }

    static QString downloadedText()
    {
        return QString::fromUtf8(u8"\u5df2\u4e0b\u8f7d");
    }

    static QString uploadedText()
    {
        return QString::fromUtf8(u8"\u5df2\u4e0a\u4f20");
    }

    static QString downloadText()
    {
        return QString::fromUtf8(u8"\u4e0b\u8f7d");
    }

    static QString uploadText()
    {
        return QString::fromUtf8(u8"\u4e0a\u4f20");
    }
};

#endif // TASKPRESENTATIONPOLICY_H
