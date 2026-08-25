#ifndef TASKFEEDBACKSUMMARY_H
#define TASKFEEDBACKSUMMARY_H

#include <QList>
#include <QString>

/**
 * @brief 任务完成反馈的纯结果汇总。
 *
 * 该对象只负责把上传/下载的终态记录转换为统计数据和展示文案，
 * 不依赖 QWidget、TaskManager 或网络对象。HomeWidge 只负责在任务
 * 状态回调和 QMessageBox 之间做页面级桥接。
 */
class TaskFeedbackSummary
{
public:
    enum class Kind {
        Download,
        Upload
    };

    enum class Outcome {
        Success,
        Failed,
        Canceled
    };

    struct Record {
        QString fileName;
        QString nodeName;
        QString type;
        Outcome outcome = Outcome::Failed;
    };

    /** 追加一个已经完成的上传或下载结果。 */
    void record(const QString &fileName,
                const QString &nodeName,
                Kind kind,
                Outcome outcome);

    /** 当前批次是否至少有一条终态记录。 */
    bool hasRecords() const;

    /** 当前批次的任务总数。 */
    int total() const;

    /** 生成与原主页弹窗一致的汇总文案。 */
    QString message() const;

    /** 清空上传、下载两类统计。 */
    void clear();

    /**
     * @brief 开始新一轮上传时重置旧实现维护的计数。
     *
     * 旧主页只清零上传 total/success/failed 三项，未触碰已保存记录和
     * canceled 字段；这里保留这一行为，避免拆分过程改变页面语义。
     */
    void resetUploadTotalsForNewBatch();

private:
    struct Stats {
        int total = 0;
        int success = 0;
        int failed = 0;
        int canceled = 0;
        QList<Record> records;
    };

    Stats m_upload;
    Stats m_download;
};

#endif // TASKFEEDBACKSUMMARY_H
