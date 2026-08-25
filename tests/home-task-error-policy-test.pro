QT += core testlib

CONFIG += testcase c++17
CONFIG -= app_bundle

INCLUDEPATH += \
    $$PWD/../src/ui/shell

SOURCES += \
    home_task_error_policy_test.cpp \
    ../src/ui/shell/HomeTaskErrorPolicy.cpp

HEADERS += \
    ../src/ui/shell/HomeTaskErrorPolicy.h
