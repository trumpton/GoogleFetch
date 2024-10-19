#-------------------------------------------------
#
# Project created by QtCreator 2019-02-14T13:14:46
#
#-------------------------------------------------

QT       += core gui network xml multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

TARGET = GoogleFetch
TEMPLATE = app


# Start Common Versioning Includes

unix:BUILDDATE = $$system(date +"%Y-%d-%m %H:%M:%S")
win32:BUILDDATE = $$system(echo "%date:~6,4%-%date:~3,2%-%date:~0,2% %time:~0,8%")

GITHASH = $$system(git --git-dir=\"$$PWD/.git\" describe --always --tags)
GITHASH = $$replace(GITHASH,"v","")
GITHASH = $$replace(GITHASH,"-",".")
LIBHASH = $$system(git --git-dir=\"$$PWD/../Lib/.git\" describe --always --tags)
LIBHASH = $$replace(LIBHASH,"v","")
LIBHASH = $$replace(LIBHASH,"-",".")

GITHASHPARTS = $$split(GITHASH, .)
V1 = $$member(GITHASHPARTS,0)
V2 = $$member(GITHASHPARTS,1)
V3 = $$member(GITHASHPARTS,2)
VERSION = "$${V1}.$${V2}.$${V3}"

system("echo VERSION=\"$$VERSION\" > $$OUT_PWD/version.txt")
system("echo GITHASH=\"$$GITHASH\" >> $$OUT_PWD/version.txt")
system("echo LIBHASH=\"$$LIBHASH\" >> $$OUT_PWD/version.txt")
system("echo BUILDDATE=\"$$BUILDDATE\" >> $$OUT_PWD/version.txt")

DEFINES += GITHASH='"\\\"$$GITHASH\\\""'
DEFINES += LIBHASH='"\\\"$$LIBHASH\\\""'
DEFINES += BUILDDATE='"\\\"$$BUILDDATE\\\""'
DEFINES += PWD='"\\\"$$PWD\\\""'

# End Common Versioning Includes

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
    account-googleaccess.h \
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
