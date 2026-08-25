QT += core network testlib

CONFIG += testcase c++17
TARGET = task_manager_state_test

win32:LIBS += -lws2_32

INCLUDEPATH += \
    $$PWD/../src/core/network \
    $$PWD/../src/features/transfers

SOURCES += \
    $$PWD/task_manager_state_test.cpp \
    $$PWD/../src/features/transfers/TaskManager.cpp \
    $$PWD/../src/features/transfers/TaskTransferGateway.cpp \
    $$PWD/../src/core/network/NetWork.cpp \
    $$PWD/../src/core/network/NodeService.cpp \
    $$PWD/../src/core/network/DirectoryService.cpp \
    $$PWD/../src/core/network/TransferService.cpp \
    $$PWD/../src/core/network/TransferServiceFileTransfer.cpp \
    $$PWD/../src/core/network/TransferControlState.cpp \
    $$PWD/../src/core/network/TransferProtocolClient.cpp

HEADERS += \
    $$PWD/../src/features/transfers/TaskManager.h \
    $$PWD/../src/features/transfers/TaskTransferGateway.h \
    $$PWD/../src/core/network/NetworkTypes.h \
    $$PWD/../src/core/network/TransferTypes.h \
    $$PWD/../src/core/network/NetWork.h \
    $$PWD/../src/core/network/NodeService.h \
    $$PWD/../src/core/network/DirectoryService.h \
    $$PWD/../src/core/network/TransferService.h \
    $$PWD/../src/core/network/TransferControlState.h \
    $$PWD/../src/core/network/TransferProtocolClient.h
