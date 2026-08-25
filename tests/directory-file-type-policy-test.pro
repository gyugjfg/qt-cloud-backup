QT += core testlib

CONFIG += testcase c++17
TARGET = directory_file_type_policy_test

INCLUDEPATH += \
    $$PWD/../src/features/directory

SOURCES += \
    $$PWD/directory_file_type_policy_test.cpp

HEADERS += \
    $$PWD/../src/features/directory/FileTypePolicy.h
