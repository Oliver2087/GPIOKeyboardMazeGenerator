QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    gameview.cpp \
    gpiocontroller.cpp \
    loadingoverlay.cpp \
    main.cpp \
    mainwindow.cpp \
    mazegenerator.cpp \
    playercontroller.cpp \
    playeritem.cpp \
    startmenu.cpp \
    textures.cpp

HEADERS += \
    gameview.h \
    gpiocontroller.h \
    loadingoverlay.h \
    mainwindow.h \
    mazegenerator.h \
    playercontroller.h \
    playeritem.h \
    startmenu.h \
    textures.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    MazeProject_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

RESOURCES += \
    resources.qrc
