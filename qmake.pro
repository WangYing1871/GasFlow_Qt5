QT +=             \
  widgets         \
  gui             \
  charts          \
  serialbus       \
  opengl          \
  serialport      \
  core

requires(qtConfig(filedialog))

QMAKE_CXXFLAGS +=                  \
  -std=c++20                       \
  -Wno-deprecated-copy             \
  -DD_develop                        \
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
