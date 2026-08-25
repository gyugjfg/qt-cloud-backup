QT += core testlib

CONFIG += testcase c++17
TARGET = transfer_control_state_test

INCLUDEPATH += \
    $$PWD/../src/core/network

SOURCES += \
    $$PWD/transfer_control_state_test.cpp \
    $$PWD/../src/core/network/TransferControlState.cpp

HEADERS += \
    $$PWD/../src/core/network/TransferControlState.h
