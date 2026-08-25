QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

gcc|clang: QMAKE_CXXFLAGS += -Wno-error

win32: LIBS += -lws2_32

# 源码、头文件和 .ui 清单集中维护；本轮只调整路径，不修改业务内容。
include(config/sources.pri)

RESOURCES += \
    resource.qrc

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
