QT += core network testlib

CONFIG += testcase c++17
TARGET = service_async_slow_network_test

INCLUDEPATH += \
    $$PWD/../src/core/network

SOURCES += \
    $$PWD/service_async_slow_network_test.cpp \
    $$PWD/../src/core/network/NodeService.cpp \
    $$PWD/../src/core/network/DirectoryService.cpp

HEADERS += \
    $$PWD/../src/core/network/NetworkTypes.h \
    $$PWD/../src/core/network/NodeService.h \
    $$PWD/../src/core/network/DirectoryService.h
