QT += core network testlib

CONFIG += testcase c++17
TARGET = directory_load_coordinator_test

INCLUDEPATH += \
    $$PWD/../src/core/network \
    $$PWD/../src/features/directory

SOURCES += \
    $$PWD/directory_load_coordinator_test.cpp \
    $$PWD/../src/features/directory/DirectoryLoadCoordinator.cpp

HEADERS += \
    $$PWD/../src/core/network/NetworkTypes.h \
    $$PWD/../src/features/directory/DirectoryLoadCoordinator.h
