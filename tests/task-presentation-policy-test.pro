QT += core testlib

CONFIG += testcase c++17
CONFIG -= app_bundle

INCLUDEPATH += \
    $$PWD/../src/core/network \
    $$PWD/../src/features/transfers

SOURCES += \
    task_presentation_policy_test.cpp

HEADERS += \
    $$PWD/../src/features/transfers/TaskPresentationPolicy.h
