/*
 * TransferService 文件传输实现：负责上传/下载协议、断点偏移和本地文件 I/O。
 * 队列调度、对象生命周期和暂停/取消状态仍由 TransferService.cpp 负责。
 */
#include "TransferService.h"

#include "TransferProtocolClient.h"
#include "TransferRequestPolicy.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

#include <memory>

bool TransferService::fileUploadResumable(const TransferRequest &request)
{
    if (m_shutdownRequested.load(std::memory_order_acquire)) {
        return false;
    }

    const QString uploadTaskId = resolvedTaskId(request, TransferKind::Upload);

    if (!validateTransferRequest(request, uploadTaskId)) {
        return false;
    }

    QFile file(request.filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emitTransferFailure(uploadTaskId, QStringLiteral("打开本地文件失败"));
        return false;
    }

    const QFileInfo fileInfo(request.filePath);
    const QString fileName = TransferProtocolClient::sanitizedFileName(fileInfo.fileName());
    const qint64 fileSize = file.size();
    const qint64 requestedOffset = TransferRequestPolicy::clampUploadOffset(request.startOffset,
                                                                              fileSize);

    int sock = TransferProtocolClient::connectToNode(request.nodeIp, request.nodePort, 10);
    if (sock < 0) {
        file.close();
        emitTransferFailure(uploadTaskId, QStringLiteral("连接节点失败"));
        return false;
    }

    {
        QMutexLocker locker(&m_taskControlMutex);
        m_transferControlState.registerSocket(TransferControlState::Direction::Upload,
                                              uploadTaskId,
                                              sock);
    }

    const QByteArray command = TransferProtocolClient::uploadCommand(fileName, fileSize, requestedOffset);
    if (!TransferProtocolClient::sendAll(sock, command.constData(), command.size())) {
        file.close();
        TransferProtocolClient::closeSocket(sock);
        return handleSocketSendFailure(uploadTaskId, true, QStringLiteral("发送上传命令失败"));
    }

    const QByteArray readyResp = TransferProtocolClient::receiveLine(sock);
    qint64 serverOffset = 0;
    if (!TransferProtocolClient::parseReadyOffset(readyResp, serverOffset)) {
        cleanupTaskControlState(uploadTaskId, true);
        file.close();
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(uploadTaskId, QStringLiteral("服务端确认失败: ") + readyResp);
        return false;
    }

    serverOffset = qBound<qint64>(0, serverOffset, fileSize);
    if (!file.seek(serverOffset)) {
        cleanupTaskControlState(uploadTaskId, true);
        file.close();
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(uploadTaskId, QStringLiteral("本地文件定位失败"));
        return false;
    }

    // 服务端已落盘完整文件时不会再要求客户端发送数据。
    // 此时 READY 已经是完成确认，不能因为随后连接关闭或 OK 回包丢失而误报失败。
    if (serverOffset >= fileSize) {
        file.close();
        TransferProtocolClient::closeSocket(sock);
        cleanupTaskControlState(uploadTaskId, true);
        emitTaskProgressSignal(uploadTaskId, 100, fileSize, fileSize, 0.0);
        emitTaskStatusSignal(uploadTaskId, static_cast<int>(NetworkTransferStatus::Completed));
        return true;
    }

    emitTaskStatusSignal(uploadTaskId, static_cast<int>(NetworkTransferStatus::Running));
    emitTaskProgressSignal(uploadTaskId,
                           fileSize > 0 ? static_cast<int>((serverOffset * 100) / fileSize) : 100,
                           serverOffset,
                           fileSize,
                           0.0);

    static const int BUF_SIZE = 1048576;
    std::unique_ptr<char[]> buffer(new char[BUF_SIZE]);
    qint64 committedOffset = serverOffset;

    QElapsedTimer timer;
    timer.start();
    qint64 lastProgressTime = 0;
    qint64 lastProgressBytes = serverOffset;

    // 上传循环里统一检查控制状态，避免暂停/取消在多个发送分支里各自收尾。
    while (true) {
        switch (checkTaskControlState(uploadTaskId, true)) {
        case TaskControlResult::Canceled:
            file.close();
            return finishCanceledTransfer(uploadTaskId, true);
        case TaskControlResult::Paused:
            file.close();
            return finishPausedTransfer(uploadTaskId, true);
        case TaskControlResult::Continue:
            break;
        }

        const qint64 bytesRead = file.read(buffer.get(), BUF_SIZE);
        if (bytesRead <= 0) {
            break;
        }

        if (!TransferProtocolClient::sendAll(sock, buffer.get(), bytesRead)) {
            file.close();
            TransferProtocolClient::closeSocket(sock);
            return handleSocketSendFailure(uploadTaskId, true, QStringLiteral("发送文件数据失败"));
        }

        committedOffset += bytesRead;
        const qint64 now = timer.elapsed();
        const int progress = fileSize > 0 ? static_cast<int>((committedOffset * 100) / fileSize) : 100;
        if (now - lastProgressTime >= 300 || committedOffset >= fileSize) {
            double speed = 0.0;
            if (now > lastProgressTime) {
                speed = ((committedOffset - lastProgressBytes) / 1024.0) /
                        ((now - lastProgressTime) / 1000.0);
            }
            emitTaskProgressSignal(uploadTaskId, progress, committedOffset, fileSize, speed);
            lastProgressTime = now;
            lastProgressBytes = committedOffset;
        }
    }

    file.close();
    const QByteArray finalResp = TransferProtocolClient::receiveLine(sock);
    TransferProtocolClient::closeSocket(sock);

    qint64 finalCommittedOffset = committedOffset;
    if (TransferProtocolClient::parseOkOffset(finalResp, finalCommittedOffset)) {
        finalCommittedOffset = qBound<qint64>(0, finalCommittedOffset, fileSize);
    }

    cleanupTaskControlState(uploadTaskId, true);

    // 数据已完整发送但最终确认包丢失时，服务端仍可能已经成功落盘。
    // 明确的 ERROR 仍然按失败处理，空响应只在本地已发送完整文件时兜底为成功。
    const bool finalResponseAccepted = finalResp.startsWith("OK");
    if (!finalResponseAccepted) {
        emitTransferFailure(uploadTaskId, QStringLiteral("服务端响应错误: ") + finalResp);
        return false;
    }

    emitTaskProgressSignal(uploadTaskId, 100, finalCommittedOffset, fileSize, 0.0);
    emitTaskStatusSignal(uploadTaskId, static_cast<int>(NetworkTransferStatus::Completed));
    return true;
}

bool TransferService::fileDownloadResumable(const TransferRequest &request)
{
    if (m_shutdownRequested.load(std::memory_order_acquire)) {
        return false;
    }

    const QString downloadTaskId = resolvedTaskId(request, TransferKind::Download);

    if (!validateTransferRequest(request, downloadTaskId)) {
        return false;
    }

    int sock = TransferProtocolClient::connectToNode(request.nodeIp, request.nodePort, 10);
    if (sock < 0) {
        emitTransferFailure(downloadTaskId, QStringLiteral("连接节点失败"));
        return false;
    }

    QByteArray cmd = TransferProtocolClient::downloadSizeCommand(request.fileName);
    if (!TransferProtocolClient::sendAll(sock, cmd.constData(), cmd.size())) {
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(downloadTaskId, QStringLiteral("发送下载命令失败"));
        return false;
    }

    const QByteArray sizeResp = TransferProtocolClient::receiveLine(sock);
    TransferProtocolClient::closeSocket(sock);
    if (sizeResp.startsWith("ERROR")) {
        emitTransferFailure(downloadTaskId, QStringLiteral("服务端错误: ") + sizeResp);
        return false;
    }

    const qint64 fileSize = sizeResp.toLongLong();
    if (fileSize == 0) {
        QFile emptyFile(request.savePath);
        if (emptyFile.open(QIODevice::WriteOnly)) {
            emptyFile.close();
        }
        emitTaskProgressSignal(downloadTaskId, 100, 0, 0, 0.0);
        emitTaskStatusSignal(downloadTaskId, static_cast<int>(NetworkTransferStatus::Completed));
        return true;
    }

    const QFileInfo existingFileInfo(request.savePath);
    const qint64 existingSize = existingFileInfo.exists() ? existingFileInfo.size() : 0;
    // 以本地实际落盘长度为恢复依据，避免任务记录偏移大于文件长度时产生空洞。
    qint64 effectiveStartOffset = TransferRequestPolicy::resolveDownloadOffset(existingSize,
                                                                                 fileSize);
    if (effectiveStartOffset >= fileSize) {
        QFile completedFile(request.savePath);
        if (completedFile.open(QIODevice::ReadWrite)) {
            completedFile.resize(fileSize);
            completedFile.close();
        }
        emitTaskProgressSignal(downloadTaskId, 100, fileSize, fileSize, 0.0);
        emitTaskStatusSignal(downloadTaskId, static_cast<int>(NetworkTransferStatus::Completed));
        return true;
    }

    emitTaskStatusSignal(downloadTaskId, static_cast<int>(NetworkTransferStatus::Running));
    emitTaskProgressSignal(downloadTaskId,
                           fileSize > 0 ? static_cast<int>((effectiveStartOffset * 100) / fileSize) : 100,
                           effectiveStartOffset,
                           fileSize,
                           0.0);

    sock = TransferProtocolClient::connectToNode(request.nodeIp, request.nodePort, 10);
    if (sock < 0) {
        emitTransferFailure(downloadTaskId, QStringLiteral("重新连接失败"));
        return false;
    }

    {
        QMutexLocker locker(&m_taskControlMutex);
        m_transferControlState.registerSocket(TransferControlState::Direction::Download,
                                              downloadTaskId,
                                              sock);
    }

    cmd = TransferProtocolClient::downloadRangeCommand(request.fileName,
                                                        effectiveStartOffset,
                                                        fileSize - 1);
    if (!TransferProtocolClient::sendAll(sock, cmd.constData(), cmd.size())) {
        cleanupTaskControlState(downloadTaskId, false);
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(downloadTaskId, QStringLiteral("发送下载请求失败"));
        return false;
    }

    const QByteArray sizeResp2 = TransferProtocolClient::receiveLine(sock);
    if (sizeResp2.startsWith("ERROR")) {
        cleanupTaskControlState(downloadTaskId, false);
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(downloadTaskId, QStringLiteral("服务端错误: ") + sizeResp2);
        return false;
    }

    if (!TransferProtocolClient::sendAll(sock, "OK\n", 3)) {
        cleanupTaskControlState(downloadTaskId, false);
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(downloadTaskId, QStringLiteral("发送确认失败"));
        return false;
    }

    QFileInfo fi(request.savePath);
    QDir dir(fi.path());
    if (!dir.exists()) {
        dir.mkpath(dir.path());
    }

    QFile file(request.savePath);
    const QIODevice::OpenMode openMode = effectiveStartOffset > 0 ? QIODevice::ReadWrite : QIODevice::WriteOnly;
    if (!file.open(openMode)) {
        cleanupTaskControlState(downloadTaskId, false);
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(downloadTaskId, QStringLiteral("打开本地文件失败"));
        return false;
    }

    if (effectiveStartOffset > 0 && !file.seek(effectiveStartOffset)) {
        cleanupTaskControlState(downloadTaskId, false);
        file.close();
        TransferProtocolClient::closeSocket(sock);
        emitTransferFailure(downloadTaskId, QStringLiteral("本地文件定位失败"));
        return false;
    }

    static const int BUF_SIZE = 1048576;
    std::unique_ptr<char[]> buffer(new char[BUF_SIZE]);
    qint64 actualCommittedBytes = effectiveStartOffset;

    QElapsedTimer timer;
    timer.start();
    qint64 lastProgressTime = 0;
    qint64 lastProgressBytes = effectiveStartOffset;

    // 下载过程始终以本地已落盘字节数作为进度基准，避免 UI 先走到终态。
    while (actualCommittedBytes < fileSize) {
        switch (checkTaskControlState(downloadTaskId, false)) {
        case TaskControlResult::Canceled:
            file.flush();
            file.close();
            return finishCanceledTransfer(downloadTaskId, false);
        case TaskControlResult::Paused:
            file.flush();
            file.close();
            return finishPausedTransfer(downloadTaskId, false);
        case TaskControlResult::Continue:
            break;
        }

        const qint64 bytesToRead = qMin<qint64>(BUF_SIZE, fileSize - actualCommittedBytes);
        const int bytesRead = TransferProtocolClient::receive(sock,
                                                               buffer.get(),
                                                               static_cast<int>(bytesToRead));
        if (bytesRead <= 0) {
            file.flush();
            file.close();
            TransferProtocolClient::closeSocket(sock);
            return handleSocketSendFailure(downloadTaskId, false, QStringLiteral("接收文件数据失败"));
        }

        const qint64 bytesWritten = file.write(buffer.get(), bytesRead);
        if (bytesWritten != bytesRead) {
            file.flush();
            file.close();
            cleanupTaskControlState(downloadTaskId, false);
            TransferProtocolClient::closeSocket(sock);
            emitTransferFailure(downloadTaskId, QStringLiteral("写入本地文件失败"));
            return false;
        }

        file.flush();
        actualCommittedBytes = qMin<qint64>(QFileInfo(request.savePath).size(), fileSize);

        const qint64 now = timer.elapsed();
        const int progress = static_cast<int>((actualCommittedBytes * 100) / fileSize);
        if (now - lastProgressTime >= 300 || actualCommittedBytes >= fileSize) {
            double speed = 0.0;
            if (now > lastProgressTime) {
                speed = ((actualCommittedBytes - lastProgressBytes) / 1024.0) /
                        ((now - lastProgressTime) / 1000.0);
            }
            emitTaskProgressSignal(downloadTaskId, progress, actualCommittedBytes, fileSize, speed);
            lastProgressTime = now;
            lastProgressBytes = actualCommittedBytes;
        }
    }

    file.flush();
    file.close();
    TransferProtocolClient::closeSocket(sock);
    cleanupTaskControlState(downloadTaskId, false);

    if (actualCommittedBytes >= fileSize) {
        emitTaskProgressSignal(downloadTaskId, 100, fileSize, fileSize, 0.0);
        emitTaskStatusSignal(downloadTaskId, static_cast<int>(NetworkTransferStatus::Completed));
        return true;
    }

    emitTransferFailure(downloadTaskId, QStringLiteral("下载不完整"));
    return false;
}
