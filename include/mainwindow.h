#ifndef mainwindow_H
#define mainwindow_H 1 
#include <iostream>
#include <string>
#include <mutex>
#include <map>
#include <deque>

#include <QObject>
#include <QMainWindow>
#include <QDialog>
#include <QCloseEvent>
#include <QChartView>

#include "component.h"
#include "run.h"
#include "ui_mainwindow.h"

#include "monitors.h"
#include "pwb.h"
#include "pwb_v1.h"


QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMenuBar;
QT_END_NAMESPACE

class mainwindow : public QMainWindow{
  Q_OBJECT

public:

  friend class run;
  mainwindow();
  ~mainwindow() noexcept = default;

  inline QSize sizeHint() const override {return QSize(400,600);}
  void fill_ports_info();
  void Monitor();
  bool is_connect();
  void update_device();
  //void clear_statusbar();

  //void init_monitors();

protected:
  void closeEvent(QCloseEvent* event) override;

  //void recive_text(size_t,std::string const&);

private:

public:
  //QMenuBar* menu_bar;
  //QMenu* menu_device, *menu_mode,* menu_tools;

  Ui::mainwindow_gui ui;
  component m_comps;
  Modbus_setting* m_modbus;
  run* m_runer;
  monitors* m_monitors;
  //qcp_monitor* m_monitors_qcp;
  pwb* m_pwb;
  pwb_v1* m_pwb_v1;
  bool _is_connected = false;
  bool _is_thread_read = false;
  bool _is_moniting = false;
  std::deque<QWidget*> m_historys;
  std::deque<bool> m_history_record;

  QLabel* m_newest;

  std::mutex m_mutex;

  std::string fname;
  bool _is_debug = true;

  //std::map<std::string,monitor*> m_monitors;

  std::vector<QChartView*> m_qcvs;
  std::vector<QWidget*> m_device_init;


public slots:

  void recive_text(size_t,std::string const&,int pos=0);
  //void Run();
  void exit();
  void browser();

  void modbus_mode(int);
  void modbus_connect_rtu();
  void modbus_disconnect_rtu();
  void Modbus_read_registers(int);
  void enable_browser(int);
  void show_pwb();
  void show_pump();
  void show_monitor();
  void show_monitor_qcp();

  void About();

  void update_statusbar(std::string const&,bool);



};



#endif
