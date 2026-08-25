QT += core testlib

CONFIG += testcase c++17
TARGET = directory_path_navigation_test

INCLUDEPATH += \
    $$PWD/../src/features/directory

SOURCES += \
    $$PWD/directory_path_navigation_test.cpp \
    $$PWD/../src/features/directory/DirectoryPathNavigation.cpp

HEADERS += \
    $$PWD/../src/features/directory/DirectoryPathNavigation.h
