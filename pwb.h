#ifndef pwb_H
#define pwb_H 1 

#include "QObject"
#include "QWidget"
#include "QDialog"
#include "QTimer"

#include "ui_pwb.h"
#include "modbus.h"

class mainwindow;

class pwb : public QWidget{
  Q_OBJECT

public:
  pwb(QWidget* parent=0);
  ~pwb() noexcept = default;
  inline QSize sizeHint() const override {return QSize(740,220);}

  Ui::pwb ui;
  modbus_t* m_modbus;
  QTimer m_timer;
  uint8_t m_coils = 0x00;

  void parent(mainwindow* value) {m_parent = value;}
  void modbus(modbus_t* value) {m_modbus = value;}


public slots:
  void enable_c0();
  void enable_c1();
  void enable_c2();
  void enable_c3();
  void set_c0();
  void set_c0_adpator();
  void count();
  void update();
  void stop();
private:
  mainwindow* m_parent;

signals:
  void aaa();

};
#endif
