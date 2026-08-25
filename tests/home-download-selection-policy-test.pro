QT += core testlib

CONFIG += testcase c++17
CONFIG -= app_bundle

INCLUDEPATH += \
    $$PWD/../src/ui/shell

SOURCES += \
    home_download_selection_policy_test.cpp \
    ../src/ui/shell/HomeDownloadSelectionPolicy.cpp

HEADERS += \
    ../src/ui/shell/HomeDownloadSelectionPolicy.h
