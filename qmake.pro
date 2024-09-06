QT +=             \
  gui             \
  charts          \
  serialbus       \
  opengl          \
  serialport      \
  core

greaterThan(QT_MAJOR_VERSION,4): QT += widgets

win32:{ CONFIG += c+=20 }
win32:{ LIBS += -lws2_32 -lsetupapi }
unix:{
  QMAKE_CXXFLAGS += -std=c++2b -Wno-deprecated-copy -Wno-deprecated-enum-enum-conversion
}
requires(qtConfig(filedialog))

QMAKE_CXXFLAGS +=                  \
  -std=c++20                       \
  -Wno-deprecated-copy             \
  -Wno-deprecated-enum-enum-conversion \
  -Wno-deprecated-declarations


CONFIG += static

HEADERS += component.h logo.h modbus_ifc.h monitors.h qcustomplot.h switch.h form.h mainwindow.h  modbus_pwb_ifc.h run.h util.h axis_tag.h glwindow.h  monitor.h simple_led.h modbus.h          modbus-rtu.h          modbus-tcp.h          modbus-version.h modbus-private.h  modbus-rtu-private.h  modbus-tcp-private.h

SOURCES += axis_tag.cpp component.cpp glwindow.cpp logo.cpp main.cpp mainwindow.cpp modbus.c modbus-data.c modbus-rtu.c modbus-tcp.c monitor.cpp monitors.cpp qcustomplot.cpp run.cpp simple_led.cpp switch.cpp util.cpp

FORMS +=mainwindow.ui  monitor.ui  qcp_monitor.ui  run.ui AHT20.ui  BMP280.ui  forward.ui  MFC.ui  Pump.ui setting_modbus.ui  Valve.ui

                       
RESOURCES += images.qrc

TARGET = GasFlow       
RESOURCES += gasflow.qrc

target.path = $$PWD

INSTALLS += target
