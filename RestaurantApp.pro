QT       += core gui widgets sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = CorrindorRestaurant
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

# Ensure all #include "something.h" resolve relative to the project root.
INCLUDEPATH += .

SOURCES += \
    admin.cpp \
    adminwindow.cpp \
    customer.cpp \
    database.cpp \
    loginwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    customerwindow.cpp \
    menuitem.cpp \
    order.cpp \
    restaurant.cpp

HEADERS += \
    admin.h \
    adminwindow.h \
    customer.h \
    database.h \
    loginwindow.h \
    mainwindow.h \
    customerwindow.h \
    menuitem.h \
    order.h \
    restaurant.h

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    mainwindow.ui

DISTFILES += \
    darktheme.qss
