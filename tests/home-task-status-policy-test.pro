QT += core testlib

CONFIG += testcase c++17
CONFIG -= app_bundle

INCLUDEPATH += \
    $$PWD/../src/ui/shell \
    $$PWD/../src/core/network

SOURCES += \
    home_task_status_policy_test.cpp

HEADERS += \
    $$PWD/../src/ui/shell/HomeTaskStatusPolicy.h \
    $$PWD/../src/core/network/TransferTypes.h
