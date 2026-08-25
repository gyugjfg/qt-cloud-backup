/**
 * @file UploadController.cpp
 * @brief 上传文件列表、勾选状态和页面步骤控制实现。
 *
 * 控制器只维护 UI 集合和稳定索引；任务创建委托给 UploadModule。大批量
 * 文件通过 GUI 事件循环分块追加，避免单次构造阻塞页面响应。
 */
#include "UploadController.h"

#include "FileItem.h"
#include "UploadModule.h"

#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QSize>
#include <QTimer>

#include <algorithm>

UploadController::UploadController(QListWidget *filesTree,
                                   QComboBox *uploadNodeCombo,
                                   QCheckBox *selectAllCheckBox,
                                   QStackedWidget *uploadStackWidget,
                                   QLabel *headerNameLabel,
                                   QLabel *headerTypeLabel,
                                   QLabel *headerSizeLabel,
                                   QLabel *headerPathLabel,
                                   UploadModule *uploadModule,
                                   QWidget *messageParent,
                                   QObject *parent)
    : QObject(parent)
    , m_filesTree(filesTree)
    , m_uploadNodeCombo(uploadNodeCombo)
    , m_selectAllCheckBox(selectAllCheckBox)
    , m_uploadStackWidget(uploadStackWidget)
    , m_headerNameLabel(headerNameLabel)
    , m_headerTypeLabel(headerTypeLabel)
    , m_headerSizeLabel(headerSizeLabel)
    , m_headerPathLabel(headerPathLabel)
    , m_uploadModule(uploadModule)
    , m_messageParent(messageParent)
{
    if (m_uploadModule) {
        connect(m_uploadModule, &UploadModule::uploadTaskCreated,
                this, &UploadController::uploadTaskCreated);
        connect(m_uploadModule, &UploadModule::switchToTaskPageRequested,
                this, &UploadController::switchToTaskPageRequested);
    }

    if (m_filesTree) {
        m_filesTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_filesTree->installEventFilter(this);
        if (m_filesTree->viewport()) {
            m_filesTree->viewport()->installEventFilter(this);
        }
        if (QScrollBar *scrollBar = m_filesTree->verticalScrollBar()) {
            connect(scrollBar, &QScrollBar::rangeChanged, this, [this]() {
                syncUploadColumnMetrics();
            });
            connect(scrollBar, &QScrollBar::valueChanged, this, [this]() {
                syncUploadColumnMetrics();
            });
        }
    }

    syncUploadColumnMetrics();
}

bool UploadController::ensureUploadNodeSelected()
{
    return m_uploadModule ? m_uploadModule->ensureUploadNodeSelected() : false;
}

/**
 * @brief 向上传页追加一条待上传文件项。
 * @param filePath 待追加的本地文件路径。
 */
void UploadController::appendUploadFileItem(const QString &filePath)
{
    if (!m_filesTree) {
        return;
    }

    QListWidgetItem *item = new QListWidgetItem(m_filesTree);
    FileItem *fileItem = new FileItem(m_filesTree);
    item->setSizeHint(fileItem->sizeHint());

    QFileInfo fileInfo(filePath);
    fileItem->setFileName(fileInfo.fileName());
    fileItem->setFileSize(fileInfo.size());
    fileItem->setFilePath(fileInfo.filePath());
    fileItem->setFileType(fileInfo.suffix(), fileInfo.fileName());

    connect(fileItem, &FileItem::checkStatusChanged, this, &UploadController::handleFileItemCheckedChanged);

    m_filesTree->addItem(item);
    m_filesTree->setItemWidget(item, fileItem);
}

void UploadController::appendUploadFiles(const QStringList &filePaths, bool switchToStepTwo)
{
    if (switchToStepTwo && !filePaths.isEmpty()) {
        switchToUploadListStep();
    }

    if (filePaths.isEmpty()) {
        return;
    }

    if (filePaths.size() <= 24) {
        m_filesTree->setUpdatesEnabled(false);
        for (const QString &filePath : filePaths) {
            appendUploadFileItem(filePath);
        }
        refreshUploadFileItemIndices();
        m_filesTree->setUpdatesEnabled(true);
        m_filesTree->viewport()->update();
        return;
    }

    m_uploadAppendInProgress = true;
    appendUploadFilesChunked(filePaths, 0);
}
// 分块追加上传文件项；每批结束后把下一批投递回 GUI 事件循环。
void UploadController::appendUploadFilesChunked(const QStringList &filePaths, int startIndex)
{
    if (!m_filesTree) {
        m_uploadAppendInProgress = false;
        return;
    }

    constexpr int kChunkSize = 18;
    const int endIndex = qMin(startIndex + kChunkSize, filePaths.size());
    m_filesTree->setUpdatesEnabled(false);
    for (int i = startIndex; i < endIndex; ++i) {
        appendUploadFileItem(filePaths.at(i));
    }
    m_filesTree->setUpdatesEnabled(true);
    m_filesTree->viewport()->update();

    if (endIndex >= filePaths.size()) {
        m_uploadAppendInProgress = false;
        refreshUploadFileItemIndices();
        return;
    }

    QTimer::singleShot(0, this, [this, filePaths, endIndex]() {
        appendUploadFilesChunked(filePaths, endIndex);
    });
}

void UploadController::refreshUploadFileItemIndices()
{
    if (!m_filesTree) {
        return;
    }

    m_checkedFileItems.clear();
    for (int i = 0; i < m_filesTree->count(); ++i) {
        QListWidgetItem *item = m_filesTree->item(i);
        FileItem *fileItem = item ? qobject_cast<FileItem*>(m_filesTree->itemWidget(item)) : nullptr;
        if (!fileItem) {
            continue;
        }

        fileItem->setItemIndex(i);
        if (fileItem->getCheckStatus() == Qt::Checked) {
            m_checkedFileItems[i] = fileItem;
        }
    }

    updateUploadSelectAllState();
    emit uploadSelectionChanged(m_checkedFileItems.size(), m_filesTree->count());
    syncUploadColumnMetrics();
}

void UploadController::setUploadFileItemsChecked(bool checked)
{
    if (!m_filesTree) {
        return;
    }

    m_filesTree->setUpdatesEnabled(false);
    m_checkedFileItems.clear();
    for (int i = 0; i < m_filesTree->count(); ++i) {
        QListWidgetItem *item = m_filesTree->item(i);
        FileItem *fileItem = item ? qobject_cast<FileItem*>(m_filesTree->itemWidget(item)) : nullptr;
        if (!fileItem) {
            continue;
        }

        const QSignalBlocker blocker(fileItem);
        fileItem->setCheckStatus(checked ? Qt::Checked : Qt::Unchecked);
        if (checked) {
            m_checkedFileItems[i] = fileItem;
        }
    }
    m_filesTree->setUpdatesEnabled(true);
    m_filesTree->viewport()->update();
    updateUploadSelectAllState();
    emit uploadSelectionChanged(m_checkedFileItems.size(), m_filesTree->count());
}

void UploadController::updateUploadSelectAllState()
{
    if (!m_selectAllCheckBox || !m_filesTree) {
        return;
    }

    const bool allChecked = m_filesTree->count() > 0 && m_checkedFileItems.size() == m_filesTree->count();
    m_selectAllCheckBox->blockSignals(true);
    m_selectAllCheckBox->setCheckState(allChecked ? Qt::Checked : Qt::Unchecked);
    m_selectAllCheckBox->blockSignals(false);
    m_selectAllCheckBox->setText(allChecked
                                     ? QStringLiteral("取消全选")
                                     : QStringLiteral("选择全部"));
}

void UploadController::clearUploadFileList()
{
    if (!m_filesTree) {
        return;
    }

    m_filesTree->clear();
    resetUploadSelectionState();
    syncUploadColumnMetrics();
}

/**
 * @brief 用当前勾选状态回收一组上传文件索引，并从页面移除。
 * @param selectedIndices 需要移除的文件索引集合。
 */
void UploadController::removeUploadFilesByIndices(const QList<int> &selectedIndices)
{
    if (!m_filesTree) {
        return;
    }

    QList<int> sortedIndices = selectedIndices;
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());

    for (int index : sortedIndices) {
        if (index < 0 || index >= m_filesTree->count()) {
            continue;
        }

        QListWidgetItem *item = m_filesTree->item(index);
        if (!item) {
            continue;
        }

        m_filesTree->removeItemWidget(item);
        delete m_filesTree->takeItem(index);
    }

    refreshUploadFileItemIndices();
    syncUploadColumnMetrics();
}

/**
 * @brief 对外接入一批本地文件，并在需要时切到上传列表页。
 * @param filePaths 待接入的本地文件路径集合。
 * @param switchToStepTwo 是否在接入后切到上传列表页。
 */
void UploadController::prepareUploadFiles(const QStringList &filePaths, bool switchToStepTwo)
{
    if (filePaths.isEmpty()) {
        return;
    }

    if (!ensureUploadNodeSelected()) {
        return;
    }

    appendUploadFiles(filePaths, switchToStepTwo);
}

QString UploadController::selectedUploadNodeId() const
{
    return m_uploadModule ? m_uploadModule->selectedUploadNodeId() : QString();
}

QList<int> UploadController::checkedUploadFileIndices() const
{
    QList<int> selectedIndices = m_checkedFileItems.keys();
    std::sort(selectedIndices.begin(), selectedIndices.end(), std::greater<int>());
    return selectedIndices;
}

void UploadController::clearUploadedSourceFiles(const QList<int> &selectedIndices)
{
    removeUploadFilesByIndices(selectedIndices);
}

void UploadController::resetUploadSelectionState()
{
    m_checkedFileItems.clear();
    if (!m_selectAllCheckBox) {
        return;
    }

    m_selectAllCheckBox->setCheckState(Qt::Unchecked);
    m_selectAllCheckBox->setText(QStringLiteral("选择全部"));
}

void UploadController::finalizeUploadAfterTaskCreation(const QList<int> &selectedIndices)
{
    clearUploadedSourceFiles(selectedIndices);
    resetUploadSelectionState();

    if (m_filesTree && m_filesTree->count() == 0 && m_uploadStackWidget) {
        m_uploadStackWidget->setCurrentIndex(0);
    }

}

void UploadController::startSelectedUploads()
{
    if (m_checkedFileItems.isEmpty()) {
        QMessageBox::information(m_messageParent,
                                 QStringLiteral("提示"),
                                 QStringLiteral("请先勾选要上传的文件"));
        return;
    }

    // 页面勾选先收成稳定文件集合，再统一交给 UploadModule 注册任务。
    const QList<int> selectedIndices = checkedUploadFileIndices();
    QStringList selectedFilePaths;
    for (int index : selectedIndices) {
        FileItem *fileItem = static_cast<FileItem*>(m_checkedFileItems.value(index));
        if (fileItem) {
            selectedFilePaths.append(fileItem->getFilePath());
        }
    }

    if (m_uploadModule) {
        m_uploadModule->createUploadTasks(selectedFilePaths);
    }
    finalizeUploadAfterTaskCreation(selectedIndices);
}

void UploadController::removeCheckedUploadFiles()
{
    removeUploadFilesByIndices(checkedUploadFileIndices());
}

void UploadController::handleFileItemCheckedChanged(bool status, QWidget *item)
{
    FileItem *fileItem = qobject_cast<FileItem*>(item);
    if (!fileItem) {
        return;
    }

    if (status) {
        m_checkedFileItems[fileItem->getItemIndex()] = item;
    } else {
        m_checkedFileItems.remove(fileItem->getItemIndex());
    }

    updateUploadSelectAllState();
    emit uploadSelectionChanged(m_checkedFileItems.size(), m_filesTree ? m_filesTree->count() : 0);
}

bool UploadController::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == m_filesTree || (m_filesTree && watched == m_filesTree->viewport())) &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest)) {
        syncUploadColumnMetrics();
    }

    return QObject::eventFilter(watched, event);
}

void UploadController::syncUploadColumnMetrics()
{
    const int rowWidth = uploadRowWidth();
    if (rowWidth <= 0) {
        return;
    }

    syncUploadRowWidths();

    FileItem::applyHeaderColumnMetrics(m_headerNameLabel,
                                       m_headerTypeLabel,
                                       m_headerSizeLabel,
                                       m_headerPathLabel,
                                       rowWidth);

    if (!m_filesTree) {
        return;
    }

    const FileItem::UploadColumnMetrics metrics = FileItem::resolveColumnMetrics(rowWidth);
    FileItem *firstFileItem = nullptr;
    for (int i = 0; i < m_filesTree->count(); ++i) {
        QListWidgetItem *item = m_filesTree->item(i);
        FileItem *fileItem = item ? qobject_cast<FileItem*>(m_filesTree->itemWidget(item)) : nullptr;
        if (!fileItem) {
            continue;
        }

        fileItem->applyColumnMetrics(metrics);
        if (!firstFileItem) {
            firstFileItem = fileItem;
        }
    }

    if (firstFileItem && m_headerNameLabel && m_headerTypeLabel && m_headerSizeLabel && m_headerPathLabel) {
        const FileItem::UploadColumnGeometry geometry = firstFileItem->currentColumnGeometry();
        m_headerNameLabel->setFixedWidth(geometry.nameWidth);
        m_headerTypeLabel->setFixedWidth(geometry.typeWidth);
        m_headerSizeLabel->setFixedWidth(geometry.sizeWidth);
        m_headerPathLabel->setFixedWidth(geometry.pathWidth);

        if (QHBoxLayout *headerLayout = m_headerNameLabel->parentWidget()
                                           ? m_headerNameLabel->parentWidget()->findChild<QHBoxLayout*>("uploadListHeaderLayout")
                                           : nullptr) {
            headerLayout->setSpacing(metrics.spacing);
            headerLayout->invalidate();
        }
    }
}

int UploadController::uploadRowWidth() const
{
    if (!m_filesTree) {
        return 0;
    }

    const int frameWidth = m_filesTree->frameWidth() * 2;
    const int viewportWidth = m_filesTree->viewport() ? m_filesTree->viewport()->width() : m_filesTree->width();
    const int widgetWidth = viewportWidth > 0 ? viewportWidth : (m_filesTree->width() - frameWidth);
    return qMax(widgetWidth, 0);
}

void UploadController::switchToUploadListStep()
{
    if (!m_uploadStackWidget) {
        return;
    }

    m_uploadStackWidget->setUpdatesEnabled(false);
    m_uploadStackWidget->setCurrentIndex(1);
    m_uploadStackWidget->setUpdatesEnabled(true);
    if (QWidget *currentPage = m_uploadStackWidget->currentWidget()) {
        currentPage->update();
    }
    m_uploadStackWidget->update();
    QTimer::singleShot(0, this, [this]() {
        syncUploadColumnMetrics();
        if (m_uploadStackWidget) {
            if (QWidget *currentPage = m_uploadStackWidget->currentWidget()) {
                currentPage->update();
            }
            m_uploadStackWidget->update();
        }
    });
}

void UploadController::syncUploadRowWidths()
{
    if (!m_filesTree) {
        return;
    }

    const int rowWidth = uploadRowWidth();
    if (rowWidth <= 0) {
        return;
    }

    for (int i = 0; i < m_filesTree->count(); ++i) {
        QListWidgetItem *item = m_filesTree->item(i);
        FileItem *fileItem = item ? qobject_cast<FileItem*>(m_filesTree->itemWidget(item)) : nullptr;
        if (!item || !fileItem) {
            continue;
        }

        const QSize rowSizeHint(rowWidth, qMax(fileItem->sizeHint().height(), 56));
        item->setSizeHint(rowSizeHint);
        fileItem->setFixedWidth(rowWidth);
    }
}
