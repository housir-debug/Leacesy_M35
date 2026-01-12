QT += quick quickcontrols2 virtualkeyboard core serialport

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/thirdparty/scpi/inc
LIBS += -L$$PWD/thirdparty/scpi/lib -lscpi -lm

HEADERS += \
    canworker.h \
    serialworker.h \
    simple_logger.h \
    vxi_11/scpimanager.h \
    vxi_11/tcpserver.h \
    vxi_11/tirpcloader.h \
    vxi_11/web_server.h

SOURCES += \
    canworker.cpp \
    main.cpp \
    serialworker.cpp \
    simple_logger.cpp \
    vxi_11/scpimanager.cpp \
    vxi_11/tcpserver.cpp \
    vxi_11/tirpcloader.cpp \
    vxi_11/web_server.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /root/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


