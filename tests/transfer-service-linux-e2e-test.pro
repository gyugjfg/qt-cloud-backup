QT += core network testlib

CONFIG += testcase c++17
TARGET = transfer_service_linux_e2e_test

win32:LIBS += -lws2_32

INCLUDEPATH += \
    $$PWD/../src/core/network

SOURCES += \
    $$PWD/transfer_service_linux_e2e_test.cpp \
    $$PWD/../src/core/network/TransferService.cpp \
    $$PWD/../src/core/network/TransferServiceFileTransfer.cpp \
    $$PWD/../src/core/network/TransferControlState.cpp \
    $$PWD/../src/core/network/TransferProtocolClient.cpp

HEADERS += \
    $$PWD/../src/core/network/TransferService.h \
    $$PWD/../src/core/network/TransferTypes.h \
    $$PWD/../src/core/network/TransferControlState.h \
    $$PWD/../src/core/network/TransferProtocolClient.h
