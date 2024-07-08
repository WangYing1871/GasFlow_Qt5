#ifndef pwb_v1_H
#define pwb_v1_H 1 

#include <vector>
#include <map>
#include <mutex>

#include "QObject"
#include "QWidget"
#include "QDialog"
#include "QTimer"
#include <QtCharts/QChartView>
#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif

#include "switch.h"
#include "ui_pwb_v1.h"
#include "monitor.h"
#include "modbus_pwb_ifc.h"
#include "modbus/modbus.h"

class mainwindow;


class pwb_v1 : public QWidget{
  Q_OBJECT

  struct data_frame_t{
    bool _is_valid = false;
    int64_t tag = 0; uint16_t data[PWB_REG_END] = {0};
    uint8_t device_coil[PWB_COIL_END] = {false};
    data_frame_t(){ }
    ~data_frame_t() noexcept = default;
  };
  data_frame_t m_data_frame;

protected:
  void closeEvent(QCloseEvent* event) override; 
  
public:
  typedef monitor::graph<QLineSeries,2> graph_t;
  typedef std::vector<graph_t> graphs_t;
  std::map<std::string,graphs_t> m_graphs;

  pwb_v1(QWidget* parent=0);
  ~pwb_v1() noexcept {}

  Ui::pwb_v1 ui;
  std::mutex m_mutex;

  mainwindow* m_parent;
  void parent(mainwindow* value) {m_parent = value;}
  void init_switch();
  void modbus_context(modbus_t* value) {m_modbus_ctx = value;}

private:
  std::map<int,SwitchButton*> m_switch_buttons;
  std::map<std::string,QTimer*> m_timers;
  modbus_t* m_modbus_ctx;

  QTimer m_timer;

  void dispatach();

  void Monitor();
  void syn_pv();

  void init_canvas();
  void draw();



public slots:
  void switch_channel0(bool);
  void switch_channel1(bool);
  void switch_channel2(bool);
  void switch_channel3(bool);
  void set_channel0();
  void set_channel1();
  void set_channel2();
  void set_channel3();
  void start();
  void stop();
  void update();
  
};

#endif
