QT += core testlib

CONFIG += testcase c++17
TARGET = directory_tab_presentation_test

INCLUDEPATH += \
    $$PWD/../src/features/directory

SOURCES += \
    $$PWD/directory_tab_presentation_test.cpp \
    $$PWD/../src/features/directory/DirectoryTabPresentation.cpp

HEADERS += \
    $$PWD/../src/features/directory/DirectoryTabPresentation.h
