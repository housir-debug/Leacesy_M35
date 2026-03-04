QT += quick quickcontrols2 virtualkeyboard core serialport

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/thirdparty/scpi/inc
LIBS += -L$$PWD/thirdparty/scpi/lib -lscpi -lm

HEADERS += \
    auxiliary/scpi_handle.h \
    auxiliary/simple_logger.h \
    auxiliary/config_manager.h \
    channel/uart_channel.h \
    vxi_11/tcp_server.h \
    vxi_11/tirpc_loader.h \
    qml/qml_agency.h \
    qml/web_server.h \
    canworker.h \
    vxi_11/uart_server.h

SOURCES += \
    auxiliary/scpi_handle.cpp \
    auxiliary/simple_logger.cpp \
    auxiliary/config_manager.cpp \
    channel/uart_channel.cpp \
    vxi_11/tcp_server.cpp \
    vxi_11/tirpc_loader.cpp \
    qml/qml_agency.cpp \
    qml/web_server.cpp \
    canworker.cpp \
    main.cpp \
    vxi_11/uart_server.cpp

RESOURCES += qml.qrc

DISTFILES += \
    auxiliary/instrument_config.ini \
    qml/Component/qmldir

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH += $$PWD/qml
# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH += $$PWD/qml

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /root/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


