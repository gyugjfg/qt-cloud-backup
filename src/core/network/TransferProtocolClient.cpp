/**
 * @file TransferProtocolClient.cpp
 * @brief fileput/filesave 协议与跨平台 socket 原语实现。
 */
#include "TransferProtocolClient.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#define TRANSFER_NATIVE_CLOSE closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define TRANSFER_NATIVE_CLOSE close
#endif

#include <cstring>

namespace {
#ifdef Q_OS_WIN
bool winsockInitialized = false;

void ensureWinsock()
{
    if (!winsockInitialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        winsockInitialized = true;
    }
}
#endif
}

int TransferProtocolClient::connectToNode(const QString &ip, int port, int timeoutSec)
{
#ifdef Q_OS_WIN
    ensureWinsock();
#endif
    const int socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle < 0) {
        return -1;
    }

#ifdef Q_OS_WIN
    const int connectTimeoutMs = timeoutSec * 1000;
    const int ioTimeoutMs = 2000;
    setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO,
               reinterpret_cast<const char *>(&connectTimeoutMs), sizeof(connectTimeoutMs));
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char *>(&ioTimeoutMs), sizeof(ioTimeoutMs));
#else
    timeval connectTimeout{timeoutSec, 0};
    timeval ioTimeout{2, 0};
    setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, &connectTimeout, sizeof(connectTimeout));
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, &ioTimeout, sizeof(ioTimeout));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.toUtf8().constData(), &address.sin_addr) != 1
        || connect(socketHandle, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        TRANSFER_NATIVE_CLOSE(socketHandle);
        return -1;
    }

    return socketHandle;
}

bool TransferProtocolClient::sendAll(int socketHandle, const char *data, qsizetype length)
{
    qsizetype sent = 0;
    while (sent < length) {
        const int chunkLength = static_cast<int>(qMin<qsizetype>(length - sent, INT_MAX));
        const int written = send(socketHandle, data + sent, chunkLength, 0);
        if (written <= 0) {
            return false;
        }
        sent += written;
    }
    return true;
}

QByteArray TransferProtocolClient::receiveLine(int socketHandle)
{
    // 每次只取一个字节，避免一次 recv 把行后的文件数据一并读走却无法回放。
    QByteArray result;
    char byte = 0;
    while (true) {
        const int received = recv(socketHandle, &byte, 1, 0);
        if (received <= 0) {
            return result.isEmpty() ? QByteArray() : result;
        }
        if (byte == '\n') {
            return result;
        }
        result.append(byte);
    }
}

int TransferProtocolClient::receive(int socketHandle, char *buffer, int length)
{
    return recv(socketHandle, buffer, length, 0);
}

void TransferProtocolClient::closeSocket(int socketHandle)
{
    if (socketHandle >= 0) {
        TRANSFER_NATIVE_CLOSE(socketHandle);
    }
}

QString TransferProtocolClient::sanitizedFileName(const QString &fileName)
{
    QString text = fileName.trimmed();
    while (!text.isEmpty()) {
        const QChar firstChar = text.front();
        if (firstChar.isLetterOrNumber() || firstChar == '.' || firstChar == '_' || firstChar == '-') {
            break;
        }
        text.remove(0, 1);
        text = text.trimmed();
    }
    return text;
}

QByteArray TransferProtocolClient::uploadCommand(const QString &fileName,
                                                 qint64 fileSize,
                                                 qint64 startOffset)
{
    return "fileput|./|" + fileName.toUtf8() + "|"
        + QString::number(fileSize).toUtf8() + "|"
        + QString::number(startOffset).toUtf8() + "\n";
}

QByteArray TransferProtocolClient::downloadSizeCommand(const QString &fileName)
{
    return "filesave|" + fileName.toUtf8() + "\n";
}

QByteArray TransferProtocolClient::downloadRangeCommand(const QString &fileName,
                                                        qint64 startOffset,
                                                        qint64 endOffset)
{
    return "filesave|" + fileName.toUtf8() + "|"
        + QString::number(startOffset).toUtf8() + "|"
        + QString::number(endOffset).toUtf8() + "\n";
}

bool TransferProtocolClient::parseReadyOffset(const QByteArray &response, qint64 &serverOffset)
{
    if (!response.startsWith("READY:")) {
        return false;
    }

    bool ok = false;
    serverOffset = response.mid(6).trimmed().toLongLong(&ok);
    return ok;
}

bool TransferProtocolClient::parseOkOffset(const QByteArray &response, qint64 &serverOffset)
{
    if (!response.startsWith("OK:")) {
        return false;
    }

    bool ok = false;
    serverOffset = response.mid(3).trimmed().toLongLong(&ok);
    return ok;
}
