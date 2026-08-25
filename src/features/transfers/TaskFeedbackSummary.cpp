#include "TaskFeedbackSummary.h"

#include <QMap>

namespace {

// 失败和取消的后缀只属于反馈文案，不向任务状态机泄漏页面字符串。
QString outcomeSuffix(const TaskFeedbackSummary::Record &record)
{
    if (record.outcome == TaskFeedbackSummary::Outcome::Success) {
        return record.nodeName;
    }

    return record.nodeName
        + (record.outcome == TaskFeedbackSummary::Outcome::Canceled
               ? QString::fromUtf8(u8" (\u5df2\u53d6\u6d88)")
               : QString::fromUtf8(u8" (\u5931\u8d25)"));
}

void appendRecords(QMap<QString, QStringList> &mergedRecords,
                   const QList<TaskFeedbackSummary::Record> &records)
{
    for (const TaskFeedbackSummary::Record &record : records) {
        mergedRecords[record.fileName].append(
            QStringLiteral("%1 - %2").arg(record.type, outcomeSuffix(record)));
    }
}

} // namespace

void TaskFeedbackSummary::record(const QString &fileName,
                                 const QString &nodeName,
                                 Kind kind,
                                 Outcome outcome)
{
    // 先形成不可变的展示记录，再更新对应方向的统计计数。
    Record record;
    record.fileName = fileName;
    record.nodeName = nodeName;
    record.type = kind == Kind::Download ? QStringLiteral("下载") : QStringLiteral("上传");
    record.outcome = outcome;

    Stats &stats = kind == Kind::Download ? m_download : m_upload;
    stats.records.append(record);
    ++stats.total;
    switch (outcome) {
    case Outcome::Success:
        ++stats.success;
        break;
    case Outcome::Canceled:
        ++stats.canceled;
        break;
    case Outcome::Failed:
        ++stats.failed;
        break;
    }
}

bool TaskFeedbackSummary::hasRecords() const
{
    return total() > 0;
}

int TaskFeedbackSummary::total() const
{
    return m_download.total + m_upload.total;
}

QString TaskFeedbackSummary::message() const
{
    // 文案规则集中在结果对象中，主页无需知道上传/下载两套计数细节。
    const int totalTasks = m_download.total + m_upload.total;
    if (totalTasks == 0) {
        return QString();
    }

    const int totalSuccess = m_download.success + m_upload.success;
    const int totalFailed = m_download.failed + m_upload.failed;
    const int totalCanceled = m_download.canceled + m_upload.canceled;

    QString message = QString::fromUtf8(u8"\u4efb\u52a1\u603b\u6570: %1    \u6210\u529f: %2    \u5931\u8d25: %3    \u53d6\u6d88: %4\n")
                          .arg(totalTasks)
                          .arg(totalSuccess)
                          .arg(totalFailed)
                          .arg(totalCanceled);
    message += QString::fromUtf8(u8"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n");

    QMap<QString, QStringList> mergedRecords;
    appendRecords(mergedRecords, m_download.records);
    appendRecords(mergedRecords, m_upload.records);

    int index = 1;
    for (auto it = mergedRecords.cbegin(); it != mergedRecords.cend(); ++it) {
        message += QStringLiteral("%1. %2\n").arg(index++).arg(it.key());
        for (const QString &detail : it.value()) {
            message += QString::fromUtf8(u8"   \u251c\u2500 %1\n").arg(detail);
        }
    }

    return message;
}

void TaskFeedbackSummary::clear()
{
    m_upload = Stats();
    m_download = Stats();
}

void TaskFeedbackSummary::resetUploadTotalsForNewBatch()
{
    m_upload.total = 0;
    m_upload.success = 0;
    m_upload.failed = 0;
}
