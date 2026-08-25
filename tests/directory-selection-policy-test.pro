QT += core testlib

CONFIG += testcase c++17
TARGET = directory_selection_policy_test

INCLUDEPATH += \
    $$PWD/../src/features/directory

SOURCES += \
    $$PWD/directory_selection_policy_test.cpp

HEADERS += \
    $$PWD/../src/features/directory/DirectorySelectionPolicy.h
