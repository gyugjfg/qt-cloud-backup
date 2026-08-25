#ifndef UPLOADCONTROLLER_H
#define UPLOADCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QMap>

class FileItem;
class QListWidget;
class QListWidgetItem;
class QComboBox;
class QCheckBox;
class QStackedWidget;
class QLabel;
class QEvent;
class UploadModule;
class QWidget;

/**
 * @brief 上传页列表与交互控制器。
 *
 * 控制器拥有上传页的列表编排职责，但不创建任务；选中的稳定文件路径
 * 交给 UploadModule。所有控件和模块均为借用指针，由主页组合根拥有，
 * 分块追加通过 GUI 事件循环继续执行，因此对象销毁时会自动取消后续回调。
 */
class UploadController : public QObject
{
    Q_OBJECT

public:
    /** 注入上传页控件和 UploadModule；控制器不拥有 Designer 控件。 */
    explicit UploadController(QListWidget *filesTree,
                              QComboBox *uploadNodeCombo,
                              QCheckBox *selectAllCheckBox,
                              QStackedWidget *uploadStackWidget,
                              QLabel *headerNameLabel,
                              QLabel *headerTypeLabel,
                              QLabel *headerSizeLabel,
                              QLabel *headerPathLabel,
                              UploadModule *uploadModule,
                              QWidget *messageParent,
                              QObject *parent = nullptr);

    /** 将节点选择校验转发给 UploadModule；模块缺失时返回 false。 */
    bool ensureUploadNodeSelected();
    /**
     * @brief 将选中的本地文件接入上传页列表。
     * @param filePaths 待接入的本地文件路径。
     * @param switchToStepTwo 是否在接入后切到上传列表页。
     */
    void prepareUploadFiles(const QStringList &filePaths, bool switchToStepTwo);
    /** 清空上传页文件项并同步勾选状态。 */
    void clearUploadFileList();
    /** 删除当前勾选的上传文件项，不触碰磁盘上的源文件。 */
    void removeCheckedUploadFiles();
    /** 设置上传文件项的统一勾选状态。 */
    void setUploadFileItemsChecked(bool checked);
    /**
     * @brief 将当前勾选文件收成稳定集合并交给上传模块注册任务。
     */
    void startSelectedUploads();
    /** 接收单个 FileItem 的勾选变化并刷新全选状态。 */
    void handleFileItemCheckedChanged(bool status, QWidget *item);
    /** 返回当前选择的上传节点 ID。 */
    QString selectedUploadNodeId() const;
    /** 返回当前勾选文件项的稳定索引，结果按降序排列以便安全删除。 */
    QList<int> checkedUploadFileIndices() const;

signals:
    /** 转发 UploadModule 创建任务的结果。 */
    void uploadTaskCreated(const QString &taskId, const QString &filePath);
    /** 上传列表勾选数量变化。 */
    void uploadSelectionChanged(int checkedCount, int totalCount);
    /** 请求主页切换到任务页。 */
    void switchToTaskPageRequested();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void switchToUploadListStep();
    void appendUploadFileItem(const QString &filePath);
    void appendUploadFiles(const QStringList &filePaths, bool switchToStepTwo);
    void refreshUploadFileItemIndices();
    void updateUploadSelectAllState();
    void removeUploadFilesByIndices(const QList<int> &selectedIndices);
    void clearUploadedSourceFiles(const QList<int> &selectedIndices);
    void resetUploadSelectionState();
    void finalizeUploadAfterTaskCreation(const QList<int> &selectedIndices);
    void syncUploadColumnMetrics();
    int uploadRowWidth() const;
    void syncUploadRowWidths();
    void appendUploadFilesChunked(const QStringList &filePaths, int startIndex);

    QListWidget *m_filesTree; ///< 借用的上传文件列表。
    QComboBox *m_uploadNodeCombo; ///< 借用的节点选择器。
    QCheckBox *m_selectAllCheckBox; ///< 借用的全选控件。
    QStackedWidget *m_uploadStackWidget; ///< 借用的上传步骤容器。
    QLabel *m_headerNameLabel; ///< 借用的列表表头控件。
    QLabel *m_headerTypeLabel; ///< 借用的列表表头控件。
    QLabel *m_headerSizeLabel; ///< 借用的列表表头控件。
    QLabel *m_headerPathLabel; ///< 借用的列表表头控件。
    UploadModule *m_uploadModule; ///< 借用的上传业务模块。
    QWidget *m_messageParent; ///< 借用的消息框父窗口。
    QMap<int, QWidget*> m_checkedFileItems; ///< 勾选行的非拥有指针缓存。
    bool m_uploadAppendInProgress = false; ///< 是否有分块追加回调等待执行。
};

#endif // UPLOADCONTROLLER_H
