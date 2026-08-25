QT += core testlib

CONFIG += testcase c++17
TARGET = transfer_contract_test

INCLUDEPATH += $$PWD/../src/core/network

SOURCES += \
    $$PWD/transfer_contract_test.cpp

HEADERS += \
    $$PWD/../src/core/network/TransferTypes.h
