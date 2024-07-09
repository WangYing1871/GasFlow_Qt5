#ifndef monitors_H
#define monitors_H 1 

//#include <pthread.h>

#include <map>
#include <vector>
#include <fstream>
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <deque>

#include <QObject>
#include <QWidget>
#include <QDialog>
#include "QPainterPath"
#include "QPainter"

#include <QStatusBar>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>
#include <QtCharts/QChartView>
#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif

#include "monitor.h"
#include "ui_monitor.h"
#include "switch.h"
#include "modbus_ifc.h"
#include "component.h"
#include "qcustomplot.h"
#include "qreadwritelock.h"

struct v_led : public QWidget{
  v_led(QWidget* parent) : QWidget(parent) {}
  QColor m_color = QColor("D0CFCC");

public:
  void setColor(QColor color) {m_color = color;}

protected:

  void paintEvent(QPaintEvent *ev) override;
  
};

class QCPRange;

class mainwindow;
class axis_tag;

class monitors : public QWidget{
  Q_OBJECT
  typedef SwitchButton switch_button_t;
public:
  //inline QSize sizeHint() const override {return QSize(1580,990);}
  monitors(mainwindow*,QWidget* parent=nullptr);
  ~monitors() noexcept{}
  mainwindow* m_parent;
  //std::map<std::string,monitor*> m_members;
  void init();
  void init_tables();

  struct data_frame_t{
    int64_t tag=0;
    uint16_t data[REG_END]={0};
    uint8_t device_coil[COIL_END] = {false};
    QReadWriteLock rw_lock;
    data_frame_t() = default;
    ~data_frame_t() noexcept = default;
    
  };
  data_frame_t m_data_frame;


  //std::vector<int64_t> m_start_times;
  QStatusBar* m_status_bar;
  int64_t m_start_time;
  std::string m_fname;
  QTimer m_timer;
  switch_button_t* m_sw;
  bool _is_enable = false;
  bool _is_start = false;
  bool _is_connect = false;
  void setfile(std::string const& v) {m_fname = v;}

  Ui::monitor ui;
  int m_mode = 0;

  std::ofstream m_fout;
  forward* m_forward;
  std::vector<v_led*> m_leds;

  //void append(std::string const&)
  
  void parent(mainwindow* v) {m_parent = v;}
public:
  void dump_mode();
  void recycle_mode();
  void mix_mode();
  void reset_forward();
  void update_forward();
  void update_mode();

public slots:
  void ConnectModbus();
  void start();
  void stop();
  void dispatach();
  void adjust_period();
  void adjust_period_flow();
  void adjust_period_recycle();
  void adjust_flow();
  void adjust_pump();
  void enable_all(bool);
  void save();
  void save_as();
  void selcet_mode();
  void reset_flow();
  void reset_pump();
  void timer_slot1();
  void get();
  void unconnect();
  void reload_serial_port();


protected:
  void closeEvent(QCloseEvent* event) override; 

public:
  typedef QCustomPlot qcp_t;
  typedef QCPGraph qcp_graph_t;
  typedef std::vector<std::pair<double,std::string>> ticks_t;
  void init_qcp();
  inline QStatusBar* status_bar() const {return m_status_bar;}

private:
  ticks_t ticks_adapt(
      QCPRange range, size_t div
      ,std::function<std::string(std::chrono::system_clock::duration,double)> trans
      );

private:
  std::mutex m_mutex, m_mutex0;
  std::atomic<bool> _is_read;
  std::atomic<bool> _is_changed;
  std::atomic<bool> _is_read_ok;
  std::atomic<bool> _is_read_new;
  std::atomic<bool> _is_record;
  std::atomic<int> m_read_error;
  std::unordered_map<std::string,std::shared_ptr<qcp_t>> qcp_plots;
  std::unordered_map<std::string,qcp_graph_t*> m_graphs;

  //max_heap
  std::unordered_map<std::string,std::deque<float>> m_caches;
  std::unordered_map<std::string,axis_tag*> m_axis_tags;
  std::unordered_map<std::string,std::shared_ptr<QTimer>> m_timers;
  QTimer mtimer, m_get_timer;
  modbus_t* m_modbus_context;
  double m_cache;
  std::chrono::system_clock::time_point m_start;
  bool _is_fs = true;
  double m_view_width = 300000;
  double m_interval = 1000.;
  std::deque<data_frame_t> data_buf;
  std::condition_variable cv_store, m_cv_lost_connect;
  std::thread record_thread;
  size_t m_index=0;

private:
  bool is_empty() const {return data_buf.size()==0;}
  bool is_full() const {return data_buf.size()==1024;}
  void to_disk_helper00();
  void log(size_t,std::string const&);

//signals:
//  void to_disk();
signals:
  void unconnect_signal();

private:
  bool connect_lost() const {return m_read_error.load()>=10;}
  void abort();


};

std::ostream& operator<<(std::ostream& os, monitors::data_frame_t const&);
//---------------------------------------------------------------------


#include "ui_qcp_monitor.h"
class qcp_monitor : public QWidget{
  Q_OBJECT

public:
  typedef qcp_monitor self_t;
  typedef QWidget base_t;
  typedef QCustomPlot qcp_t;
  typedef QCPGraph qcp_graph_t;
  typedef std::vector<std::pair<double,std::string>> ticks_t;
  struct data_frame_t{
    int64_t tag=0;
    uint16_t data[REG_END]={0};
    uint8_t device_coil[COIL_END] = {false};
    QReadWriteLock rw_lock;
    data_frame_t() = default;
    ~data_frame_t() noexcept = default;
  };

public:
  qcp_monitor(mainwindow* parent, QWidget* sub=nullptr);
  ~qcp_monitor() noexcept {}

  std::map<std::string,qcp_t*> qcp_plots;
  mainwindow* m_parent;

  //std::shared_ptr<qcp_graph_t> m_graph0;
  //std::shared_ptr<qcp_graph_t> m_graph1;
  qcp_graph_t* m_graph0;
  qcp_graph_t* m_graph1;
  qcp_graph_t* m_graph2;

  axis_tag* mtag1,* mtag2,* mtag3;
  QTimer mtimer;
  data_frame_t m_data_frame;

  std::mutex m_mutex;
  
  std::atomic<bool> _is_read;

  void get();






  Ui::qcp_monitor ui;

  double m_index;
  double m_cache;
  std::chrono::system_clock::time_point m_start;
  bool _is_fs = true;
  double m_view_width = 300000;
  double m_interval = 1000.;



private slots:
  void timer_slot1();
  void start();
  void stop();

  void aaa();
  void aaa(
      QCPLegend::SelectableParts
      
      ) {info_out("aaa: SelectableParts");}
  void aaa(QMouseEvent*,bool,QVariant const&,bool*);

private:
  ticks_t ticks_adapt(
      QCPRange range, size_t div
      ,std::function<std::string(std::chrono::system_clock::duration,double)> trans
      );
protected:
  void closeEvent(QCloseEvent* event) override; 

  

};
#endif
