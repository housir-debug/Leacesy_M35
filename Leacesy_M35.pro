QT += quick quickcontrols2 virtualkeyboard core serialport

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/thirdparty/scpi/inc
LIBS += -L$$PWD/thirdparty/scpi/lib -lscpi -lm

HEADERS += \
    canworker.h \
    scpimanager.h \
    serialworker.h \
    simple_logger.h \
    tcpserver.h \
    tirpcloader.h

SOURCES += \
        canworker.cpp \
        main.cpp \
        scpimanager.cpp \
        serialworker.cpp \
        simple_logger.cpp \
        tcpserver.cpp \
        tirpcloader.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /root/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


