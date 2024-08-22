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

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/include/modbus

CONFIG += \
  static

HEADERS +=  \
  $$PWD/include/*.h \
  $$PWD/include/modbus/*.h
 

SOURCES +=  \
  $$PWD/src/*.cpp \
  $$PWD/src/*.c

FORMS +=                     \
  $$PWD/ui/*.ui              \
  $$PWD/ui/components/*.ui 
                       
RESOURCES += images.qrc

TARGET = GasFlow       
RESOURCES += gasflow.qrc

target.path = $$PWD

INSTALLS += target
