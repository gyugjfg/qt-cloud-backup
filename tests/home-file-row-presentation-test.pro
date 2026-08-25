QT += core testlib

CONFIG += testcase c++17
CONFIG -= app_bundle

INCLUDEPATH += \
    $$PWD/../src/core/network \
    $$PWD/../src/ui/shell

SOURCES += \
    home_file_row_presentation_test.cpp \
    ../src/ui/shell/HomeFileRowPresentation.cpp

HEADERS += \
    ../src/core/network/NetworkTypes.h \
    ../src/ui/shell/HomeFileRowPresentation.h
