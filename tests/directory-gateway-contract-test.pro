QT += core network testlib

CONFIG += testcase c++17
TARGET = directory_gateway_contract_test

win32:LIBS += -lws2_32

INCLUDEPATH += \
    $$PWD/../src/core/network

SOURCES += \
    $$PWD/directory_gateway_contract_test.cpp \
    $$PWD/../src/core/network/DirectoryGateway.cpp \
    $$PWD/../src/core/network/NetWork.cpp \
    $$PWD/../src/core/network/NodeService.cpp \
    $$PWD/../src/core/network/DirectoryService.cpp \
    $$PWD/../src/core/network/TransferService.cpp \
    $$PWD/../src/core/network/TransferServiceFileTransfer.cpp \
    $$PWD/../src/core/network/TransferControlState.cpp \
    $$PWD/../src/core/network/TransferProtocolClient.cpp

HEADERS += \
    $$PWD/../src/core/network/DirectoryGateway.h \
    $$PWD/../src/core/network/NetworkTypes.h \
    $$PWD/../src/core/network/TransferTypes.h \
    $$PWD/../src/core/network/NetWork.h \
    $$PWD/../src/core/network/NodeService.h \
    $$PWD/../src/core/network/DirectoryService.h \
    $$PWD/../src/core/network/TransferService.h \
    $$PWD/../src/core/network/TransferControlState.h \
    $$PWD/../src/core/network/TransferProtocolClient.h
