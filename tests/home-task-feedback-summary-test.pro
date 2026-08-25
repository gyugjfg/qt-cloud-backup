QT += core testlib
CONFIG += testcase c++17
CONFIG -= app_bundle

INCLUDEPATH += ../src/features/transfers

SOURCES += \
    home_task_feedback_summary_test.cpp \
    ../src/features/transfers/TaskFeedbackSummary.cpp

HEADERS += \
    ../src/features/transfers/TaskFeedbackSummary.h
