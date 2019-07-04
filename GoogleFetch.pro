#-------------------------------------------------
#
# Project created by QtCreator 2019-02-14T13:14:46
#
#-------------------------------------------------

QT       += core gui network xml multimedia
win32:QT += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GoogleFetch
TEMPLATE = app

# Include Git Hash / Revision Details
# Uncomment NOGIT if Git not used.
#DEFINES += NOGIT
GITHASH = \\\"$$system(git --git-dir=\"$$PWD/.git\" describe --always --tags)\\\"
LIBHASH = \\\"$$system(git --git-dir=\"$$PWD/../Lib/.git\" describe --always --tags)\\\"
DEFINES += GITHASH=$$GITHASH
DEFINES += LIBHASH=$$LIBHASH
DEFINES += PWD=\\\"$$PWD\\\"

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    googleconnection.cpp \
    ../Lib/supportfunctions.cpp \
    ../Lib/warningok.cpp \
    ../Lib/warningyesno.cpp \
    ../Lib/alertsound.cpp

HEADERS += \
        mainwindow.h \
    google-account.h \
    googleconnection.h \
    ../Lib/supportfunctions.h \
    ../Lib/warningok.h \
    ../Lib/warningyesno.h \
    ../Lib/alertsound.h 

FORMS += \
        mainwindow.ui \
    ../Lib/encryption.ui \
    ../Lib/warningok.ui \
    ../Lib/warningyesno.ui

RESOURCES += \
    ../Lib/sounds.qrc
