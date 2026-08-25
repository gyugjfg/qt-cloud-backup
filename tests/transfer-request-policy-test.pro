QT += core testlib

CONFIG += testcase c++17
TARGET = transfer_request_policy_test

INCLUDEPATH += \
    $$PWD/../src/core/network

SOURCES += \
    $$PWD/transfer_request_policy_test.cpp

HEADERS += \
    $$PWD/../src/core/network/TransferRequestPolicy.h
