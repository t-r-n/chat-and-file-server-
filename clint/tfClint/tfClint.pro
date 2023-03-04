QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


INCLUDEPATH += E:\boost_1_79_0\boost_1_79_0
LIBS += -LE:\boost_1_79_0\boost_1_79_0\stage\lib
#LIBS += -lpthread libwsock32 libws2_32 #加了这句静态编译会出错,不加动态编译出错 不要加-lpthread这个因该是linux上的
#LIBS += libwsock32 libws2_32 #这样静态动态编译都没问题了
LIBS += -lpthread libwsock32 libws2_32

#LIBS += libws2_32 #这样静态动态编译都没问题了

#QT +=testlib
#win32:QMAKE_LFLAGS += -shared

SOURCES += \
    adddialog.cpp \
    clint.cpp \
    dialog.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    adddialog.h \
    clint.h \
    dialog.h \
    widget.h

FORMS += \
    dialog.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    imagee.qrc
