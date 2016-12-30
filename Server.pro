#-------------------------------------------------
#
# Project created by QtCreator 2016-12-30T12:53:17
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Server
TEMPLATE = app

RC_FILE = uac.rc
#引入第三方库计算JWT
LIBS += -L"C:\jwt" -llibchilkat-9.5.0
INCLUDEPATH += $$quote(C:\jwt/include)

SOURCES += main.cpp\
        mainwindow.cpp \
    Class/bucket.cpp \
    Class/cloudinfo.cpp \
    Class/jwt.cpp \
    Class/migration.cpp \
    Class/systemtime.cpp \
    CloudSDK/aliyunclient.cpp \
    CloudSDK/azureclient.cpp \
    CloudSDK/baiduyunclient.cpp \
    CloudSDK/cloudclient.cpp \
    CloudSDK/googleclient.cpp \
    CloudSDK/s3client.cpp

HEADERS  += mainwindow.h \
    Class/bucket.h \
    Class/cloudinfo.h \
    Class/jwt.h \
    Class/migration.h \
    Class/systemtime.h \
    CloudSDK/aliyunclient.h \
    CloudSDK/azureclient.h \
    CloudSDK/baiduyunclient.h \
    CloudSDK/cloudclient.h \
    CloudSDK/googleclient.h \
    CloudSDK/s3client.h

FORMS    += mainwindow.ui

RESOURCES += \
    images/qrc.qrc

