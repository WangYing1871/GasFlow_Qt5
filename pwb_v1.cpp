#include <iostream>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <stdexcept>
#include "mainwindow.h"
#include "pwb.h"
#include "util.h"

#include <QTime>
#include <QValueAxis>
#include <QChart>
#include <QtCharts/QChartView>
#include <QGraphicsLayout>

pwb_v1::pwb_v1(QWidget* p):
  QWidget(p){
  ui.setupUi(this);

  this->setWindowTitle("高压板");

  m_timers.insert(std::make_pair("monitor",new QTimer));
  m_timers.insert(std::make_pair("dispatach",new QTimer));
  m_timers["monitor"]->setInterval(1000);
  m_timers["dispatach"]->setInterval(1000);

  connect(m_timers["monitor"],&QTimer::timeout,this,&pwb_v1::Monitor);
  connect(m_timers["dispatach"],&QTimer::timeout,this,&pwb_v1::dispatach);

  ui.lcdNumber->setDigitCount(8);
  //ui.lcdNumber_2->setDigitCount(10);

  ui.lineEdit->setEnabled(false);
  ui.lineEdit_2->setEnabled(false);
  ui.lineEdit_3->setEnabled(false);
  ui.lineEdit_4->setEnabled(false);
  ui.lineEdit_5->setEnabled(false);
  ui.lineEdit_6->setEnabled(false);
  ui.lineEdit_7->setEnabled(false);
  ui.lineEdit_8->setEnabled(false);
  init_switch();
  init_canvas();
}

void pwb_v1::init_switch(){
  auto* sbt0 = new SwitchButton(ui.widget);
  connect(sbt0,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel0(bool)));
  auto* sbt1 = new SwitchButton(ui.widget_3);
  connect(sbt1,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel1(bool)));
  auto* sbt2 = new SwitchButton(ui.widget_4);
  connect(sbt2,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel2(bool)));
  auto* sbt3 = new SwitchButton(ui.widget_2);
  connect(sbt3,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel3(bool)));
  m_switch_buttons.insert(std::make_pair(0,sbt0));
  m_switch_buttons.insert(std::make_pair(1,sbt1));
  m_switch_buttons.insert(std::make_pair(2,sbt2));
  m_switch_buttons.insert(std::make_pair(3,sbt3));
  for (auto&& [_,x] : m_switch_buttons ) x->setGeometry(0,0,69,25);

  connect(ui.pushButton,&QAbstractButton::clicked,this,&pwb_v1::set_channel0);
  connect(ui.pushButton_2,&QAbstractButton::clicked,this,&pwb_v1::set_channel1);
  connect(ui.pushButton_3,&QAbstractButton::clicked,this,&pwb_v1::set_channel2);
  connect(ui.pushButton_4,&QAbstractButton::clicked,this,&pwb_v1::set_channel3);

  connect(ui.pushButton_5,&QAbstractButton::clicked,this,&pwb_v1::start);
  connect(ui.pushButton_6,&QAbstractButton::clicked,this,&pwb_v1::stop);

  ui.checkBox->setEnabled(false); ui.checkBox_2->setEnabled(false);
  ui.checkBox_3->setEnabled(false); ui.checkBox_4->setEnabled(false);
  ui.lineEdit_15->setEnabled(false); ui.lineEdit_16->setEnabled(false);
  ui.lineEdit_17->setEnabled(false); ui.lineEdit_18->setEnabled(false);

  connect(ui.pushButton_8,&QAbstractButton::clicked,this,&pwb_v1::update);

  
}

void pwb_v1::update(){
  Monitor();
  dispatach();
}


#define function_register_switch(name,index)            \
void pwb_v1::name(bool v){                              \
  auto* md_ctx = m_parent->m_modbus->m_modbus_context;  \
  if (!m_parent->is_connect()){ m_parent->recive_text(  \
      2,"connect device please"); return; }             \
  int ec = modbus_write_bit(md_ctx,index,v);            \
  if (ec != 1){                                         \
    std::stringstream sstr;                             \
    sstr<<"channel "<<(index-4)<<" switch to "<<v<<     \
      " Failed";                                        \
    m_parent->recive_text(1,sstr.str());                \
    m_switch_buttons[index-4]->switch_to(               \
        !m_switch_buttons[index-4]->state());           \
    return; }                                           \
  std::stringstream sstr;                               \
  sstr<<"channel "<<(index-4)<<" switch to "<<v<<" Ok"; \
  m_parent->recive_text(0,sstr.str()); }                \
/**/
function_register_switch(switch_channel0,4)
function_register_switch(switch_channel1,5)
function_register_switch(switch_channel2,6)
function_register_switch(switch_channel3,7)
#undef function_register_switch


#define function_register_set(name,from,REG)                                      \
void pwb_v1::name(){                                                              \
  std::stringstream sstr(ui.from->text().toStdString().c_str());                  \
  uint16_t value; sstr>>std::hex>>value>>std::dec;                                \
  if (!sstr.fail()){                                                              \
    auto* md_ctx = m_parent->m_modbus->m_modbus_context;                          \
    int ec = modbus_write_register(md_ctx,REG,value);                             \
    if (ec != 1){ m_parent->recive_text(1,"set channel value Failed"); return; }  \
    std::stringstream sstr; sstr<<#name<<" to value "<<value<<" Ok";              \
    m_parent->recive_text(0,sstr.str());                                          \
  }else{ std::stringstream sstr;                                                  \
    m_parent->recive_text(1,"please input grammar as 'hex'(0x0000-0xFFFF)"); } }  \
/**/
function_register_set(set_channel0,lineEdit_12,PWB_REG_SVV_CH0)
function_register_set(set_channel1,lineEdit_13,PWB_REG_SVV_CH1)
function_register_set(set_channel2,lineEdit_14,PWB_REG_SVV_CH2)
function_register_set(set_channel3,lineEdit_11,PWB_REG_SVV_CH3)


void pwb_v1::Monitor(){
  if (!m_parent->is_connect()) return;
  auto* md_ctx = m_parent->m_modbus->m_modbus_context;

  std::unique_lock<std::mutex> lock(m_mutex);
  int ec = modbus_read_registers(md_ctx,0,PWB_REG_END,m_data_frame.data);
  lock.unlock();
  if (ec != PWB_REG_END){ m_parent->recive_text(1,"PWB read registers failed"); return; }
  util::t_msleep(20);

  lock.lock();
  ec = modbus_read_bits(md_ctx,0,PWB_COIL_END,m_data_frame.device_coil);
  lock.unlock();
  if (ec != PWB_COIL_END){ m_parent->recive_text(1,"PWB read coils failed"); return; } }



void pwb_v1::start(){
  if (!m_parent->is_connect()) return;
  m_timers["monitor"]->start();
  m_timers["dispatach"]->start();
  
}
void pwb_v1::syn_pv(){
  std::lock_guard<std::mutex> lock(m_mutex);
  m_switch_buttons[0]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH0]);
  m_switch_buttons[1]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH1]);
  m_switch_buttons[2]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH2]);
  m_switch_buttons[3]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH3]);
}

void pwb_v1::stop(){
  m_timers["monitor"]->stop();
  m_timers["dispatach"]->stop();
}
void pwb_v1::dispatach(){

  //for(auto&& x : m_data_frame.data) debug_out(x);
  //for(auto&& x : m_data_frame.device_coil) debug_out(x);

  uint32_t uuid = 0;
  meta::encode(uuid,util::b2i_indx
      ,m_data_frame.data[PWB_REG_BOARD_UUID]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+1]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+2]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+3]
      );
  ui.lcdNumber_2->display((int)uuid);

  uint32_t seconds = 0;
  meta::encode(seconds,util::u162u32_indx
      ,m_data_frame.data[PWB_REG_HEARTBEAT_HI],m_data_frame.data[PWB_REG_HEARTBEAT_LW]);
  QString ts{util::uint2timestr(seconds).c_str()};
  ui.lcdNumber->display(ts);

  syn_pv();
  ui.checkBox->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH0]==1 ? Qt::Checked : Qt::Unchecked);
  ui.checkBox_2->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH1]==1 ? Qt::Checked : Qt::Unchecked);
  ui.checkBox_4->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH2]==1 ? Qt::Checked : Qt::Unchecked);
  ui.checkBox_3->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH3]==1 ? Qt::Checked : Qt::Unchecked);

  ui.lineEdit_18->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH0]).c_str());
  ui.lineEdit_17->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH1]).c_str());
  ui.lineEdit_16->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH2]).c_str());
  ui.lineEdit_15->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH3]).c_str());

  ui.lineEdit_9->setText(QString::number(
        util::get_bmp280_1(m_data_frame.data+PWB_REG_TEMPERATURE_INT)));
  ui.lineEdit_10->setText(QString::number(
        util::get_bmp280_1(m_data_frame.data+PWB_REG_HUMIDITY_INT)));

  std::unique_lock<std::mutex> lock(m_mutex);
  ui.lineEdit  ->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH0]).c_str());
  ui.lineEdit_4->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH1]).c_str());
  ui.lineEdit_5->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH2]).c_str());
  ui.lineEdit_7->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH3]).c_str());
  ui.lineEdit_2->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH0]).c_str());
  ui.lineEdit_3->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH1]).c_str());
  ui.lineEdit_6->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH2]).c_str());
  ui.lineEdit_8->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH3]).c_str());
  lock.unlock();
}


void pwb_v1::init_canvas(){
  //auto* c0_x = new QValueAxis; c0_x->setTickCount(1);
  //auto* c1_x = new QValueAxis; c1_x->setTickCount(1);
  //auto* c2_x = new QValueAxis; c2_x->setTickCount(1);
  //auto* c3_x = new QValueAxis; c3_x->setTickCount(1);
  //auto* c0_yv = new QValueAxis; c0_yv->setLinePenColor("#FC0316");
  //auto* c1_yv = new QValueAxis; c1_yv->setLinePenColor("#FC0316");
  //auto* c2_yv = new QValueAxis; c2_yv->setLinePenColor("#FC0316");
  //auto* c3_yv = new QValueAxis; c3_yv->setLinePenColor("#FC0316");
  //auto* c0_yi = new QValueAxis; c0_yi->setLinePenColor("#2F03FC");
  //auto* c1_yi = new QValueAxis; c1_yi->setLinePenColor("#2F03FC");
  //auto* c2_yi = new QValueAxis; c2_yi->setLinePenColor("#2F03FC");
  //auto* c3_yi = new QValueAxis; c3_yi->setLinePenColor("#2F03FC");

  //std::vector<std::pair<std::string,QChart*>> charts;

  //auto* chart_c0 = new QChart();
  //charts.emplace_back(std::make_pair("V&I",chart_c0));
  //auto* c0_lv = new QLineSeries(chart_c0);
  //auto* c0_li = new QLineSeries(chart_c0);
  //chart_c0->addAxis(c0_x,Qt::AlignBottom);
  //chart_c0->addAxis(c0_yv,Qt::AlignLeft);
  //chart_c0->addAxis(c0_yi,Qt::AlignRight);
  //chart_c0->addSeries(c0_lv); chart_c0->addSeries(c0_li);
  //c0_lv->attachAxis(c0_x); c0_lv->attachAxis(c0_yv);
  //c0_li->attachAxis(c0_x); c0_li->attachAxis(c0_yi);


  //auto* chart_c1 = new QChart();
  //charts.emplace_back(std::make_pair("V&I",chart_c1));
  //auto* c1_lv = new QLineSeries(chart_c1);
  //auto* c1_li = new QLineSeries(chart_c1);
  //chart_c1->addAxis(c1_x,Qt::AlignBottom);
  //chart_c1->addAxis(c1_yv,Qt::AlignLeft);
  //chart_c1->addAxis(c1_yi,Qt::AlignRight);
  //chart_c1->addSeries(c1_lv); chart_c1->addSeries(c1_li);
  //c1_lv->attachAxis(c1_x); c1_lv->attachAxis(c1_yv);
  //c1_li->attachAxis(c1_x); c1_li->attachAxis(c1_yi);

  //auto* chart_c2 = new QChart();
  //charts.emplace_back(std::make_pair("V&I",chart_c2));
  //auto* c2_lv = new QLineSeries(chart_c2);
  //auto* c2_li = new QLineSeries(chart_c2);
  //chart_c2->addAxis(c2_x,Qt::AlignBottom);
  //chart_c2->addAxis(c2_yv,Qt::AlignLeft);
  //chart_c2->addAxis(c2_yi,Qt::AlignRight);
  //chart_c2->addSeries(c2_lv); chart_c2->addSeries(c2_li);
  //c2_lv->attachAxis(c2_x); c2_lv->attachAxis(c2_yv);
  //c2_li->attachAxis(c2_x); c2_li->attachAxis(c2_yi);

  //auto* chart_c3 = new QChart();
  //charts.emplace_back(std::make_pair("V&I",chart_c3));
  //auto* c3_lv = new QLineSeries(chart_c3);
  //auto* c3_li = new QLineSeries(chart_c3);
  //chart_c3->addAxis(c3_x,Qt::AlignBottom);
  //chart_c3->addAxis(c3_yv,Qt::AlignLeft);
  //chart_c3->addAxis(c3_yi,Qt::AlignRight);
  //chart_c3->addSeries(c3_lv); chart_c3->addSeries(c3_li);
  //c3_lv->attachAxis(c3_x); c3_lv->attachAxis(c3_yv);
  //c3_li->attachAxis(c3_x); c3_li->attachAxis(c3_yi);

  //for(auto&& [_,x] : charts)
  //  x->setTitle(_.c_str()), x->legend()->hide(),
  //  x->setAnimationOptions(QChart::AllAnimations),
  //  x->setMargins(QMargins(0,0,0,0)),
  //  x->layout()->setContentsMargins(0,0,0,0);
  //ui.graphicsView->setChart(chart_c0); ui.graphicsView->setRenderHint(QPainter::Antialiasing);
  //ui.graphicsView_2->setChart(chart_c1); ui.graphicsView_2->setRenderHint(QPainter::Antialiasing);
  //ui.graphicsView_3->setChart(chart_c2); ui.graphicsView_3->setRenderHint(QPainter::Antialiasing);
  //ui.graphicsView_4->setChart(chart_c3); ui.graphicsView_4->setRenderHint(QPainter::Antialiasing);

  

  //auto* c0_lv = new QLineSeries
  //graph_t gr_c0v; gr_c0v
}
void draw(){

}

void pwb_v1::closeEvent(QCloseEvent* event){
  Q_UNUSED(event);
  for (auto&& [_,y] : m_timers)
    if (y->isActive()) y->stop();
}
