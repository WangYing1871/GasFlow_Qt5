#ifndef form_H
#define form_H 1 

#include <QString>

namespace form{
static QString const group_box_font0 = "\
  QGroupBox{ \
     font-size: 14px; \
     font-weight: bold; \
     font-style: italic }\
   QGroupBox::title{ \
     subcontrol-origin: margin; \
     subcontrol-position: top left; \
     color: blue; \
     padding: 0 3px; \
     font-size: 6px; \
     font-family: Arial; \
   }\
   ";
static QString const group_box_font1 = "\
  QGroupBox{ \
     font-size: 14px; \
     font-weight: bold; \
     font-style: italic }\
   QGroupBox::title{ \
     subcontrol-origin: margin; \
     subcontrol-position: top left; \
     color: black; \
     padding: 0 3px; \
     font-size: 6px; \
     font-family: Arial; \
   }\
   ";

static QString const value_axis_title = "\
  QGroupBox{              \
    font-size: 12px;      \
    font-weight: blod;    \
  }                       \
  QValueAxis::title{      \
    subcontrol-position: button right; \
    color: black;         \
    font-size: 6pt;       \
    find-family: Arial;   \
  }                       \
   ";
}
#endif

