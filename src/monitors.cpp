#include <iostream>
#include <cmath>
#include <memory>
#include <iomanip>
#include <functional>
#include <condition_variable>

#include <QGraphicsLayout>
#include <QValueAxis>
#include <QMessageBox>
#include <QtWidgets/QLineEdit>
#include <QFileDialog>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QPen>
//#include <QDataTime>

#include "glwindow.h"

#include "monitors.h"
#include "mainwindow.h"

#include "util.h"
#include "form.h"
#include "modbus_ifc.h"
#include "axis_tag.h"
namespace{

//QSharedPointer<QCPAxisTickerText> axis_tick00(QCPAxis* axis,size_t count,std::string unit=""){
void axis_tick00(QCPAxis* axis,size_t count,std::string unit=""){
  QSharedPointer<QCPAxisTickerText> rt(new QCPAxisTickerText);
  auto rg = axis->range();
  for (size_t i=0; i<count-1; ++i){
    float value = rg.lower + (i+1)*rg.size()/(float)count;
    std::stringstream sstr; sstr<<std::setprecision(2)<<value;
    rt->addTick(value,sstr.str().c_str());
  }
  std::stringstream sstr;
  sstr<<std::setprecision(2)<<rg.upper<<" "<<unit;
  rt->addTick(rg.upper,sstr.str().c_str());
  axis->setTicker(rt);
}

constexpr static uint16_t const CS_DUMP_MODE = 1;
constexpr static uint16_t const CS_RECYCLE_MODE = 2;
constexpr static uint16_t const CS_MIX_MODE = 3;

std::map<uint16_t,std::string> mode_names = {
  {1,"Dump"}
  ,{2,"Recycle"}
  ,{3,"Mix"}
};
std::map<uint16_t,std::string> state_names = {
  {1,"Dump"}
  ,{2,"Recycle"}
};


std::string mode_name(uint16_t i){
  if(auto iter = mode_names.find(i); iter != mode_names.end()) return iter->second;
  return "X"; }
QCPRange range_trans00(QCPRange r){
  QCPRange rt;
  rt.lower = r.lower + 0.8*(r.size());
  rt.upper = r.upper + 0.8*(r.size());
  return rt;
}
QCPRange range_trans01(QCPRange r, double in){
  QCPRange rt;
  rt.lower = r.lower + in;
  rt.upper = r.upper + in;
  return rt;
}
QCPRange range_trans02(QCPAxis* axis, float v,bool& c){
  QCPRange r = axis->range();
  QCPRange rt(r.lower,r.upper);
  c = false;
  if ((v-r.lower)>=r.size()*0.95) 
    //rt.upper = v+20, rt.lower = rt.upper-r.size(), c=true;
    rt.upper = v+20,  rt.upper = 10*((int)rt.upper/10), rt.lower = 0, c=true;
  else if ((v-r.lower)<=0.2*r.size()){
    if (r.upper>=20)
    rt.upper -= 20, c = true;
  }
  return rt;
}

void syn_axis(QCPAxis* from, QCPAxis* to){
  to->setRange(from->range().lower,from->range().upper);
  to->setTicker(from->ticker());
}

void unable_progress_bar(QProgressBar* v){
  v->setRange(0,0);
  QPalette p;
  p.setColor(QPalette::Highlight, QColor(Qt::lightGray));
  v->setPalette(p);
}

}

void v_led::paintEvent(QPaintEvent*){
  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
  painter.save();
  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(m_color));
  //painter.drawEllipse(QRect(0,0,size().width(),size().height()));
  //painter.drawEllipse(0,0,100,100);
  painter.restore();
}


monitors::monitors(mainwindow* parent,QWidget* sub):
  QWidget(sub){
  ui.setupUi(this);

  ui.groupBox->setStyleSheet(form::group_box_font0);
  ui.groupBox_2->setStyleSheet(form::group_box_font0);
  ui.groupBox_3->setStyleSheet(form::group_box_font0);
  ui.groupBox_4->setStyleSheet(form::group_box_font0);

  

  /*
  QSurfaceFormat fmt;
  fmt.setDepthBufferSize(24);
  if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
    qDebug("Requesting 3.3 core context");
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);}
  else { qDebug("Requesting 3.0 context"); fmt.setVersion(3, 0); }
  QSurfaceFormat::setDefaultFormat(fmt);

  //GLWindow glwindow
  */
  
  

  //m_parent = parent;
  //parent->setEnabled(false);
  //this->resize(1600,950);
  //this->setGeometry(10,10,1600,990);
  this->setGeometry(10,10,1380,955);

  connect(ui.pushButton,&QAbstractButton::clicked,this,&monitors::start);
  connect(ui.pushButton_3,&QAbstractButton::clicked,this,&monitors::stop);
  connect(ui.pushButton_4,&QAbstractButton::clicked,this,&monitors::save_as);
  connect(ui.pushButton_6,&QAbstractButton::clicked,this,&monitors::adjust_period_flow);
  connect(ui.pushButton_5,&QAbstractButton::clicked,this,&monitors::adjust_period_recycle);
  connect(ui.pushButton_7,&QAbstractButton::clicked,this,&monitors::adjust_flow);
  connect(ui.pushButton_8,&QAbstractButton::clicked,this,&monitors::adjust_pump);
  connect(ui.pushButton_9,&QAbstractButton::clicked,this,&monitors::selcet_mode);
  connect(ui.pushButton_12,&QAbstractButton::clicked,this,&monitors::reset_flow);
  connect(ui.pushButton_13,&QAbstractButton::clicked,this,&monitors::reset_pump);

  connect(&mtimer,&QTimer::timeout,this,&monitors::timer_slot1);

  connect(&m_timer,&QTimer::timeout,this,&monitors::dispatach);
  m_timer.setInterval(1000);
  m_forward = new forward(ui.widget);
  m_forward->ui.groupBox->setStyleSheet(form::group_box_font0);
  m_forward->ui.groupBox_2->setStyleSheet(form::group_box_font0);
  m_forward->ui.groupBox_3->setStyleSheet(form::group_box_font0);

  //m_forward->ui.groupBox_3->setStyleSheet(form::group_box_font1);
  init();
  init_qcp();
  init_tables();

  //m_forward->ui.lcdNumber->setDigitCount(8);
  m_forward->ui.lcdNumber->setDigitCount(2);
  m_forward->ui.lcdNumber_2->setDigitCount(2);
  m_forward->ui.lcdNumber_3->setDigitCount(2);
  m_forward->ui.lcdNumber_4->setDigitCount(2);

  connect(this,&monitors::unconnect_signal,this,&monitors::unconnect);
  //unable_progress_bar(m_forward->ui.progressBar);
}

void monitors::unconnect(){
  stop();
}


void monitors::init_qcp(){
  ui.widget_2->resize(909,326);
  ui.widget_3->resize(909,326);

  auto* qcp0 = new qcp_t(ui.widget_2);
  qcp_plots.insert(std::make_pair("sensors",qcp0));
  qcp0->resize(ui.widget_2->size().width(),ui.widget_2->size().height());
  qcp0->yAxis->setTickLabels(false);
  connect(qcp0->yAxis2,SIGNAL(rangeChanged(QCPRange))
      ,qcp0->yAxis,SLOT(setRange(QCPRange)));
  qcp0->yAxis2->setVisible(true);
  qcp0->axisRect()->addAxis(QCPAxis::atRight);
  qcp0->axisRect()->axis(QCPAxis::atRight,0)->setPadding(30);
  qcp0->axisRect()->axis(QCPAxis::atRight,1)->setPadding(30);
  qcp0->xAxis->setRange(0,m_view_width);
  qcp0->axisRect()->axis(QCPAxis::atRight,0)->setRange(-10,50);
  qcp0->axisRect()->axis(QCPAxis::atRight,1)->setRange(95,105);

  auto* qcp_graph_t = qcp0->addGraph(qcp0->xAxis
      ,qcp0->axisRect()->axis(QCPAxis::atRight,0));
  QPen p0(QColor(250,120,0));
  p0.setWidth(2.5);
  qcp_graph_t->setPen(p0);
  auto* qcp_graph_p = qcp0->addGraph(qcp0->xAxis
      ,qcp0->axisRect()->axis(QCPAxis::atRight,1));
  //auto* qcp_graph_p = qcp0->addGraph(qcp0->xAxis
  //    ,qcp0->axisRect()->axis(QCPAxis::atLeft,0));
  QPen p1(QColor(0,180,60));
  p1.setWidth(2.5);
  qcp_graph_p->setPen(p1);

  QSharedPointer<QCPAxisTickerText> ttx( new QCPAxisTickerText);
  m_start = std::chrono::system_clock::now();
  for(auto&& x : ticks_adapt(qcp0->xAxis->range(),5,&util::format_time)){
    ttx->addTick(x.first,x.second.c_str()); }
  qcp0->xAxis->setTicker(ttx);
  qcp0->xAxis->setSelectableParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  qcp0->legend->setVisible(true);
  auto font_sub = font();
  qcp0->legend->setFont(font_sub);
  qcp0->legend->setBrush(QBrush(QColor(255,255,255,230)));

  auto* qcp1 = new qcp_t(ui.widget_3);
  qcp_plots.insert(std::make_pair("flow",qcp1));
  qcp1->resize(ui.widget_3->size().width(),ui.widget_3->size().height());
  qcp1->axisRect()->addAxis(QCPAxis::atRight);
  qcp1->axisRect()->axis(QCPAxis::atRight,0)->setPadding(30);
  qcp1->axisRect()->axis(QCPAxis::atRight,1)->setPadding(30);
  qcp1->axisRect()->axis(QCPAxis::atRight,1)->setVisible(false);
  qcp1->axisRect()->axis(QCPAxis::atRight,0)->setTickLabels(true);
  qcp1->yAxis->setTickLabels(false);
  qcp1->legend->setVisible(true);
  qcp1->legend->setBrush(QBrush(QColor(255,255,255,230)));
  qcp1->xAxis->setRange(qcp0->xAxis->range().lower,qcp0->xAxis->range().upper);
  qcp1->xAxis->setTicker(ttx);
  connect(qcp1->yAxis2,SIGNAL(rangeChanged(QCPRange))
      ,qcp1->axisRect()->axis(QCPAxis::atRight,0),SLOT(setRange(QCPRange)));
  qcp1->yAxis2->setVisible(true);
  qcp1->axisRect()->axis(QCPAxis::atRight,0)->setRange(0,60);

  //axis_tick00(qcp1->axisRect()->axis(QCPAxis::atRight,0),6,"ccm");
  //axis_tick00(qcp0->axisRect()->axis(QCPAxis::atRight,0),5,"°C");
  //axis_tick00(qcp0->axisRect()->axis(QCPAxis::atRight,1),5,"kPa");

  auto* qcp_graph_f = 
    qcp1->addGraph(qcp1->xAxis,qcp1->axisRect()->axis(QCPAxis::atRight,0));
  QPen p2(QColor(0,180,60));
  p2.setWidth(2.5);
  qcp_graph_f->setPen(p2);
  //m_graphs.insert(std::make_pair("Temperature/°C",qcp_graph_t));
  m_graphs.insert(std::make_pair("Temperature (°C)",qcp_graph_t));
  m_graphs.insert(std::make_pair("Pressure (kPa)",qcp_graph_p));
  m_graphs.insert(std::make_pair("Flow-Rate (ccm)",qcp_graph_f));

  for (auto&& [x,y] : m_graphs){
    y->setName(x.c_str());
    auto* at = new axis_tag(y->valueAxis());
    auto gp = y->pen(); gp.setWidth(1); at->setPen(gp);
    m_axis_tags.insert(std::make_pair(x,at));
  }

  connect(&mtimer,SIGNAL(timeout()),this,SLOT(timer_slot1()));
  mtimer.setInterval(m_interval);

  connect(&m_get_timer,SIGNAL(timeout()),this,SLOT(get()));
  m_get_timer.setInterval(m_interval);

  m_timers.insert(std::make_pair("mix_dump_timer",new QTimer{}));
  m_timers.insert(std::make_pair("mix_recycle_timer",new QTimer{}));

  _is_changed.store(true);
  _is_read_ok.store(false);
  _is_read_new.store(false);
  _is_record.store(false);


  connect(ui.pushButton_5,&QAbstractButton::clicked,this,[this](){ _is_changed.store(true); });
  connect(ui.pushButton_6,&QAbstractButton::clicked,this,[this](){ _is_changed.store(true); });
  connect(ui.pushButton_7,&QAbstractButton::clicked,this,[this](){ _is_changed.store(true); });
  connect(ui.pushButton_8,&QAbstractButton::clicked,this,[this](){ _is_changed.store(true); });

  connect(ui.pushButton_2,&QAbstractButton::clicked,this
      ,&monitors::ConnectModbus);



  //connect(this,&monitors::to_disk,this,&monitors::to_disk_impl);

}

void monitors::ConnectModbus(){
  if (!_is_connect){
    std::string device = ui.comboBox_2->currentText().toStdString();
    int slave_addr = ui.spinBox->value();
    auto cs = util::connect_modbus(device.c_str(),19200,'N',8,2,slave_addr);
    if (cs.second){
      _is_connect = true;
      ui.pushButton_2->setText("close");
      ui.comboBox_2->setEnabled(false);
      ui.spinBox->setEnabled(false);
      m_modbus_context = cs.first;
    util::info_to<QTextBrowser,util::log_mode::k_info>(
        ui.textBrowser_3,
        "Link modbus device Ok.", " device: ", device
        ,"baud: ",19200
        ,"parity: ","None"
        ,"data_bit: ",8
          ,"stop_bit: ",2
          ,"slave_addr: ",slave_addr
          );
    }else{
      _is_connect = false;
      util::info_to<QTextBrowser,util::log_mode::k_error>(
          ui.textBrowser_3,
          "Link modbus device failed!");
    }
  }else{
    modbus_close(m_modbus_context); modbus_free(m_modbus_context);
    util::info_to<QTextBrowser,util::log_mode::k_info>(ui.textBrowser_3
        ,"disconnect modbus serial, Bye");
    ui.spinBox->setEnabled(true);
    ui.comboBox_2->setEnabled(true);
    ui.pushButton_2->setText("open");
    _is_connect = false;
  }
}


void monitors::save_as(){
}

void monitors::timer_slot1(){
  using namespace std::chrono;
  auto dr = duration_cast<system_clock::duration>(
      system_clock::now().time_since_epoch()
      -m_start.time_since_epoch()).count();
  double tt = dr/1.e+6;

  auto rx = qcp_plots["sensors"]->xAxis->range();
  if (tt >= rx.lower+0.8*rx.size()){
    auto nr = range_trans01(rx,m_interval);
    QSharedPointer<QCPAxisTickerText> ttx(
        new QCPAxisTickerText);
    for (auto&& x : ticks_adapt(nr,5,&util::format_time))
      ttx->addTick(x.first,x.second.c_str());
    qcp_plots["sensors"]->xAxis->setRange(nr.lower,nr.upper);
    qcp_plots["sensors"]->xAxis->setTicker(ttx);
    qcp_plots["flow"]->xAxis->setRange(nr.lower,nr.upper);
    qcp_plots["flow"]->xAxis->setTicker(ttx);
  }

  if (_is_read_ok.load() && _is_read_new.load()){
    m_data_frame.rw_lock.lockForRead();
    update_forward();
    m_mode = 
      m_data_frame.data[REG_CUR_MODE]==1 ? 0 :
      m_data_frame.data[REG_CUR_MODE]==2 ? 2 :
      m_data_frame.data[REG_CUR_MODE]==3 ? 2 : 3;
    float yt = util::get_bmp280_1(
        m_data_frame.data+REG_TEMPERATURE_DEC);
    float pt = util::get_bmp280_1(
        m_data_frame.data+REG_PRESSURE_DEC);
    float pc = util::to_float(m_data_frame.data+REG_MFC_RATE_PV);
    m_data_frame.rw_lock.unlock();

    auto* axis = qcp_plots["flow"]->axisRect()->axis(QCPAxis::atRight,0);
    auto rg0 = axis->range();
    bool is_c;
    axis->setRange(range_trans02(axis,pc,is_c));
    


    m_graphs["Temperature (°C)"]->addData(tt,yt);
    m_graphs["Pressure (kPa)"]->addData(tt,pt);
    m_graphs["Flow-Rate (ccm)"]->addData(tt,pc);
    m_cache = tt;

    for (auto&& [x,y] : m_axis_tags){
      double buf = m_graphs[x]->dataMainValue(m_graphs[x]->dataCount()-1);
      y->updatePosition(buf);
      y->setText(QString::number(buf,'f',2));
    }
    _is_read_new.store(false);
  }
  //debug_out("==================");
  //debug_out(tt);
  //debug_out(qcp_plots["sensors"]->xAxis->range().lower);
  //debug_out(qcp_plots["sensors"]->xAxis->range().upper);

  for (auto&& [x,y] : qcp_plots) y->replot();
}

void monitors::log(size_t mode,std::string const& value){
  auto* active = ui.textBrowser_3;
  mode==0 ?  util::info_to<QTextBrowser,util::log_mode::k_info>(active,value) : 
    mode==1 ?  util::info_to<QTextBrowser,util::log_mode::k_warning>(active,value) : 
      mode==2 ?  util::info_to<QTextBrowser,util::log_mode::k_error>(active,value) :
        util::info_to<QTextBrowser,util::log_mode::k_error>(active,"unknow content mode, display as ERROR!");
}




void monitors::reset_flow(){
  auto* ctx = m_modbus_context;
  modbus_write_bit(ctx,COIL_DEFAULT_RATE_MFC,1)==1 ? 
    log(0,"Reset MFC-Rate ok.") : log(1,"Reset MFC-Rate failed"); }
void monitors::reset_pump(){
  auto* ctx = m_modbus_context;
  modbus_write_bit(ctx,COIL_DEFAULT_SPEED_PUMP,1)==1 ?
    log(0,"Reset pump speed ok.") : log(1,"Reset pump speed failed"); }

void monitors::init_tables(){
}

void monitors::save(){
}

  
void monitors::dump_mode(){
  auto* ctx = m_modbus_context;
  int ec = modbus_write_register(ctx,REG_SET_MODE,CS_DUMP_MODE);
  if (ec != 1){ log(2,"set mode to dump Failed"); return; }
  log(0,"set mode to dump Ok"); }
void monitors::recycle_mode(){
  auto* ctx = m_modbus_context;
  int ec = modbus_write_register(ctx,REG_SET_MODE,CS_RECYCLE_MODE);
  if (ec != 1){ log(2,"set mode to recycle Failed"); return; }
  log(0,"set mode to recycle Ok"); }
void monitors::mix_mode(){
  auto* ctx = m_modbus_context;
  int ec = modbus_write_register(ctx,REG_SET_MODE,CS_MIX_MODE);
  if (ec != 1){ log(2,"set mode to mix Failed"); return; }
  log(0,"set mode to mix Ok"); }

void monitors::selcet_mode(){
  std::string mode_name = ui.comboBox->currentText().toStdString();
  if (mode_name=="Dump") dump_mode();
  else if (mode_name=="Recycle") recycle_mode();
  else if (mode_name=="Mix") mix_mode(); }

void monitors::init(){
  for (auto&& x : util::get_ports_name())
    ui.comboBox_2->addItem(x.c_str());
  ui.spinBox->setRange(1,255);
  ui.spinBox->setSingleStep(1);
  //for (auto&& x : m_parent->m_comps.m_components){ x.second->close(); }
  std::vector<std::pair<std::string,monitor*>> ms; 

  m_forward->ui.textBrowser->setFontPointSize(26);
  m_forward->ui.textBrowser->setAlignment(Qt::AlignJustify);
  m_forward->ui.textBrowser_2->setFontPointSize(26);
  m_forward->ui.textBrowser_2->setAlignment(Qt::AlignJustify);
  //Qt::Alignment ali = ui.textBrowser->alignment();
  //m_forward->ui.textBrowser->setAlignment(ali);
  //m_forward->ui.textBrowser_2->setAlignment(ali);

  m_forward->ui.textBrowser->setTextColor(Qt::green);
  m_forward->ui.textBrowser_2->setTextColor(Qt::green);
}

void monitors::start(){
  if (!_is_connect){
    log(1,"Please connect first"); return; }

  ui.pushButton->setEnabled(false); 
  ui.pushButton_3->setEnabled(true);
  ui.pushButton_2->setEnabled(false);
  m_fname = util::time_to_str()+".csv";
  m_fout = std::ofstream(m_fname.c_str());
  m_fout<<"# Record time: "<<util::highrestime_ns()<<"\n# timestamp,  ";
  size_t as = std::extent<decltype(util::item_names)>::value;
  for (size_t i=0; i<as-1; ++i) m_fout<<util::item_names[i];
  m_fout<<util::item_names[as-1]<<"\n";
  if (_is_fs){
    m_start = std::chrono::system_clock::now();
    _is_fs = false;
    for (auto&& [x,y] : m_graphs)
      if (y->data()->size() != 0) y->data()->clear();
    for (auto&& [x,y] : qcp_plots) y->replot();
  }
  _is_read.store(true);
  QSharedPointer<QCPAxisTickerText> ttx( new QCPAxisTickerText);
  for(auto&& x : ticks_adapt(qcp_plots["sensors"]->xAxis->range(),5,&util::format_time)){
    ttx->addTick(x.first,x.second.c_str()); }
  qcp_plots["sensors"]->xAxis->setTicker(ttx);
  qcp_plots["flow"]->xAxis->setTicker(ttx);

  //std::unique_lock lock;
  //m_cv_lost_connect.wait(lock,std::bind(&monitors::connect_lost,this));
  m_get_timer.start();

  mtimer.start();

  //_is_record.store(true);
  //record_thread = std::thread([this](){this->to_disk_helper00();});
  //record_thread.join();
}

void monitors::abort(){
  std::unique_lock lock(m_mutex);
  m_cv_lost_connect.wait(lock,std::bind(&monitors::connect_lost,this));
  mtimer.stop();
  m_get_timer.stop();
  m_read_error.store(0);
  ui.pushButton_3->setEnabled(false);
  ui.pushButton->setEnabled(true);
  lock.unlock();
  //QMessageBox msgBox;
  //msgBox.setText("The document has been modified.");
  //msgBox.setInformativeText("Do you want to save your changes?");
  //msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  //msgBox.setDefaultButton(QMessageBox::Save);
  //int ret = msgBox.exec();
}


void monitors::to_disk_helper00(){
  debug_out("to_disk_helper00 start");
  while(_is_record.load()){
    if (data_buf.size()>=20){
      std::unique_lock lock(m_mutex0);
      //for (int i=0; i<20; ++i){
      //  m_fout<<data_buf.front(); data_buf.pop_front; }
      lock.unlock();
    }
    util::t_msleep(500);
  }
  debug_out("to_disk_helper00 end");
}
void monitors::stop(){
  m_fout.flush();
  _is_read.store(false);
  _is_record.store(false);
  m_get_timer.stop();
  mtimer.stop();
  ui.pushButton_3->setEnabled(false);
  ui.pushButton->setEnabled(true);
  ui.pushButton_2->setEnabled(true);
}

void monitors::update_mode(){
  m_mode = 
    m_data_frame.data[REG_CUR_MODE]==1 ? 0 :
    m_data_frame.data[REG_CUR_MODE]==2 ? 1 :
    m_data_frame.data[REG_CUR_MODE]==3 ? 2 : 3;
  ui.comboBox->setCurrentIndex(m_mode);
  ui.comboBox->update();

  ui.lineEdit->setText(std::to_string(m_data_frame.data[REG_FLOW_DURATION]).c_str());
  ui.lineEdit_4->setText(std::to_string(m_data_frame.data[REG_RECYCLE_DURATION]).c_str());
  std::stringstream sstr; sstr<<std::setprecision(5)<<util::to_float(m_data_frame.data+REG_MFC_RATE_SV);
  ui.lineEdit_2->setText(sstr.str().c_str());
  ui.lineEdit_3->setText(std::to_string(m_data_frame.data[REG_PUMP_SPEED_SV]).c_str());
}

void monitors::dispatach(){
  update_forward();
}
void monitors::update_forward(){
  uint32_t seconds = 0;
  meta::encode(seconds,util::u162u32_indx
      ,m_data_frame.data[REG_HEARTBEAT_HI],m_data_frame.data[REG_HEARTBEAT_LW]);
  QString strs[4];
  for (size_t index = 0; auto&& x : util::uint2_dhms(seconds))
    strs[index++] = QString(x.c_str());
  m_forward->ui.lcdNumber->display(strs[0]);
  m_forward->ui.lcdNumber_2->display(strs[1]);
  m_forward->ui.lcdNumber_3->display(strs[2]);
  m_forward->ui.lcdNumber_4->display(strs[3]);

  m_forward->ui.textBrowser->setText(mode_name(m_data_frame.data[REG_CUR_MODE]).c_str());
  m_forward->ui.textBrowser_2->setText(
      state_names[m_data_frame.data[REG_CUR_STATE]].c_str());

  auto const& dp = [](bool v)->std::string{
    if (v) return "background: #88FA36;";
    else return "background: #0E1A50;"; };

  //v_led mfc_led(m_forward->ui.widget);
  //v_led mfc_pump(m_forward->ui.widget_3);
  //v_led mfc_valve(m_forward->ui.widget_2);
  //mfc_led.setColor(dp(m_data_frame.device_coil[0]).c_str());
  //mfc_led.update();


  m_forward->ui.widget->setStyleSheet(dp(m_data_frame.device_coil[0]).c_str());
  m_forward->ui.widget_2->setStyleSheet(dp(m_data_frame.device_coil[2]).c_str());
  m_forward->ui.widget_3->setStyleSheet(dp(m_data_frame.device_coil[1]).c_str());
  m_forward->ui.widget->update();
  m_forward->ui.widget_2->update();
  m_forward->ui.widget_3->update();

  /*
  if (_is_changed.load()){
    std::stringstream sstr; sstr<<std::setprecision(5)<<util::to_float(m_data_frame.data+REG_MFC_RATE_SV);
    m_forward->ui.label_13->setText(sstr.str().c_str());
    m_forward->ui.label_12->setText(std::to_string(m_data_frame.data[REG_PUMP_SPEED_SV]).c_str());
    m_forward->ui.label_11->setText(std::to_string(m_data_frame.data[REG_FLOW_DURATION]).c_str());
    m_forward->ui.label_14->setText(std::to_string(m_data_frame.data[REG_RECYCLE_DURATION]).c_str());
    _is_changed.store(false);
  }
  */
}


void monitors::closeEvent(QCloseEvent*){
  m_fout.close();
  if (_is_read.load()) _is_read.store(false);
  mtimer.stop();
  for (auto&& [x,y] : m_timers) y->stop();
  for (auto&& [x,y] : m_graphs) y->data()->clear();
  ui.pushButton->setEnabled(true);
  ui.pushButton_3->setEnabled(true);
}

void monitors::enable_all(bool v){
  _is_enable = v;
  v ? log(0,">>>>>>>>>>>>>>>>>>>RUN MODE IN================") :
    log(0,"===============RUN MODE OUT<<<<<<<<<<<<<<<");
  auto* ctx = m_modbus_context;
  int indexs[] = { COIL_ONOFF_MFC ,COIL_ONOFF_PUMP ,COIL_ONOFF_VALVE };
  for (auto&& x : indexs) modbus_write_bit(ctx,x,v); }

void monitors::reset_forward(){
  m_forward->ui.textBrowser->setTextColor("#1F1D1D");
  m_forward->ui.textBrowser_2->setTextColor("#1F1D1D");
  m_forward->ui.textBrowser->setText("X");
  m_forward->ui.textBrowser_2->setText("X");
  
  m_forward->ui.widget->setStyleSheet("background: gray;");
  m_forward->ui.widget_2->setStyleSheet("background: gray;");
  m_forward->ui.widget_3->setStyleSheet("background: gray");
  //QFile styleSheet(":/files/nature_1.jpg");
  //if (!styleSheet.open(QIODevice::ReadOnly)) {
  //    qWarning("Unable to open :/files/nostyle.qss");}
  //else{
  //  m_forward->ui.widget->setStyleSheet(styleSheet.readAll());
  //}

  

  m_forward->ui.widget->update();
  m_forward->ui.widget_2->update();
  m_forward->ui.widget_3->update();


}

monitors::ticks_t monitors::ticks_adapt(
      QCPRange range, size_t div
      ,std::function<std::string(std::chrono::system_clock::duration,double)> trans){
  if(div<=1) return ticks_t{};
  ticks_t rt;
  double del = range.size()/div;
  for (double i=range.lower; i<=range.upper; i+=del)
    rt.emplace_back(std::make_pair(i
          ,trans(m_start.time_since_epoch()+std::chrono::milliseconds((int)range.lower),i)));
  return rt;
}
void monitors::adjust_period(){
}
void monitors::adjust_period_flow(){
  std::stringstream text(ui.lineEdit->text().toStdString().c_str());
  uint16_t v;
  text>>v;
  if (text.fail()){
    QMessageBox::warning(this,tr("Wraning")
        ,"Invalid input for set flow duration, format: 'uint16_t'");
    return; }
  auto* ctx = m_modbus_context;
  int ec = modbus_write_register(ctx,REG_FLOW_DURATION,v);
  if (ec!=1){
    log(1,"set flow duration Failed");
    return; }
  text.clear(); text<<"set flow duration "<<v<<" Ok";
  log(0,text.str());
}
void monitors::adjust_period_recycle(){
  std::stringstream text(ui.lineEdit_4->text().toStdString().c_str());
  uint16_t v;
  text>>v;
  if (text.fail()){
    QMessageBox::warning(this,tr("Wraning")
        ,"Invalid input for set recycle duration, format: 'uint16_t'");
    return; }
  auto* ctx = m_modbus_context;
  int ec = modbus_write_register(ctx,REG_RECYCLE_DURATION,v);
  if (ec!=1){
    log(1,"set recycle duration Failed");
    return; }
  text.clear(); text<<"set recycle duration "<<v<<" Ok";
  log(0,text.str());
}

void monitors::adjust_flow(){
  std::stringstream text(ui.lineEdit_2->text().toStdString().c_str());
  float f; text>>f;
  if (text.fail()){
    text.clear();
    text<<"set flow in 'float' format ["<<ui.lineEdit_2->text().toStdString()<<"]";
    log(1,text.str()); return; }
  uint16_t* addr = reinterpret_cast<uint16_t*>(&f);
  uint16_t zero[2] = {addr[0],addr[1]};
  auto* ctx = m_modbus_context;
  int ec = modbus_write_registers(ctx,REG_MFC_RATE_SV,2,zero);
  if (ec==2){text.clear(); text<<"update flow to "<<f<<" Ok"; log(0,text.str());}
  else {log(2,"adjust flow Failed");}
}

void monitors::adjust_pump(){
  std::stringstream text(ui.lineEdit_3->text().toStdString().c_str());
  uint16_t p; text>>p;
  if (text.fail()) {
    log(1,"please input uint16_t format");
    return;
  }
  Pump* ptr =  dynamic_cast<Pump*>(m_parent->m_comps.at("Pump"));
  
  std::stringstream sstr; 
  if (p<ptr->m_range[0] && p>ptr->m_range[1]){
    sstr<<"please input uint16_t in range ["<<
      ptr-> m_range[0]<<","<<ptr->m_range[1]<<"]";
    log(1,sstr.str());
    return; }
  sstr.clear();
  modbus_t* mbs_ctx = m_modbus_context;
  if (modbus_write_register(mbs_ctx,REG_PUMP_SPEED_SV,p)==1){
    sstr<<"set pwm pump Ok. valve "<<p/(float)ptr->m_range[1]<<"%";
    log(0,sstr.str().c_str()); return; }
  sstr<<"set pwm pump Failed";
  log(2,sstr.str().c_str()); }

void monitors::get(){

  //std::unique_lock lock(m_mutex);
  //cv_store.wait(lock,std::bind(&monitors::is_full,this));

  auto* mb_context = m_modbus_context;
  m_data_frame.rw_lock.lockForWrite();
  int ec0 = modbus_read_registers(mb_context
      ,0,REG_END,&m_data_frame.data[0]);
  int ec1 = modbus_read_bits(mb_context
      ,0,COIL_END,&m_data_frame.device_coil[0]);
  m_data_frame.tag = util::highrestime_ns();
  m_data_frame.rw_lock.unlock();
  //lock.unlock();
  if (ec0!=REG_END || ec1!=COIL_END){
    log(2,"update status failed");
    m_read_error++;
    if(m_read_error.load()==15) m_cv_lost_connect.notify_one();
    _is_read_ok.store(false);
  }else{
    _is_read_ok.store(true);
    _is_read_new.store(true);

    m_fout<<m_data_frame;
    if (m_index++==20){ m_fout.flush(); m_index=0; }
    //std::unique_lock lock(m_mutex0);
    //data_buf.push_back(m_data_frame);
    //lock.unlock();
  }
}


std::ostream& operator<<(std::ostream& os, monitors::data_frame_t const& df){
  for (int i=0; i<REG_END-1; ++i) os<<df.data[i]<<" "; os<<df.data[REG_END-1]<<"\n";
  return os;
}
//---------------------------------------------------------------------
qcp_monitor::qcp_monitor(mainwindow* p
    ,QWidget* sub):
  QWidget(sub){
  ui.setupUi(this);
  m_parent = p;
  _is_read.store(false);
  auto* qcp0 = new qcp_t(ui.widget_2);
  qcp_plots.insert(std::make_pair("000",qcp0));
  ui.widget_2->resize(909,392);
  qcp0->resize(ui.widget_2->size().width(),ui.widget_2->size().height());

  qcp0->yAxis->setTickLabels(false);
  connect(qcp0->yAxis2,SIGNAL(rangeChanged(QCPRange))
      ,qcp0->yAxis,SLOT(setRange(QCPRange)));
  qcp0->yAxis2->setVisible(true);

  qcp0->axisRect()->addAxis(QCPAxis::atRight);
  qcp0->axisRect()->axis(QCPAxis::atRight,0)->setPadding(30);
  qcp0->axisRect()->axis(QCPAxis::atRight,1)->setPadding(30);

  m_graph0 = qcp0->addGraph(qcp0->xAxis
      ,qcp0->axisRect()->axis(QCPAxis::atRight,0));
  QPen p0(QColor(250,120,0));
  p0.setWidth(2.5);
  m_graph0->setPen(p0);

  m_graph1 = qcp0->addGraph(qcp0->xAxis
      ,qcp0->axisRect()->axis(QCPAxis::atRight,1));
  QPen p1(QColor(0,180,60));
  p1.setWidth(2.5);
  m_graph1->setPen(p1);

  m_graph0->setName("Temperature");
  m_graph1->setName("Pressure");

  mtag1 = new axis_tag(m_graph0->valueAxis());
  auto gp0 = m_graph0->pen(); gp0.setWidth(1);
  mtag1->setPen(gp0);
  mtag2 = new axis_tag(m_graph1->valueAxis());
  auto gp1 = m_graph1->pen(); gp1.setWidth(1);
  mtag2->setPen(gp1);

  mtag2->setPen(m_graph1->pen());

  connect(&mtimer,SIGNAL(timeout()),this,SLOT(timer_slot1()));
  mtimer.setInterval(m_interval);

  connect(ui.pushButton,&QAbstractButton::clicked
      ,this,&qcp_monitor::start);
  connect(ui.pushButton_3,&QAbstractButton::clicked
      ,this,&qcp_monitor::stop);

  connect(qcp0->xAxis,SIGNAL(rangeChanged(QCPRange))
      ,qcp0->xAxis,SLOT(setRange(QCPRange)));

  qcp0->xAxis->setRange(0,m_view_width);
  qcp0->axisRect()->axis(QCPAxis::atRight,0)->setRange(-10,50);
  qcp0->axisRect()->axis(QCPAxis::atRight,1)->setRange(95,105);

  m_index = 0.;

  QSharedPointer<QCPAxisTickerText> ttx( new QCPAxisTickerText);
  m_start = std::chrono::system_clock::now();
  for(auto&& x : ticks_adapt(qcp0->xAxis->range(),5,&util::format_time)){
    ttx->addTick(x.first,x.second.c_str()); }
  qcp0->xAxis->setTicker(ttx);
  qcp0->xAxis->setSelectableParts(QCPAxis::spAxis|QCPAxis::spTickLabels);

  qcp0->legend->setVisible(true);
  auto font_sub = font();
  qcp0->legend->setFont(font_sub);
  qcp0->legend->setBrush(QBrush(QColor(255,255,255,230)));

  QObject::connect(qcp0->legend,SIGNAL(selectionChanged(QCPLegend::SelectableParts))
      ,this,SLOT(aaa(QCPLegend::SelectableParts)));

  auto* qcp1 = new qcp_t(ui.widget_3);
  qcp_plots.insert(std::make_pair("001",qcp1));
  ui.widget_3->resize(909,392);
  qcp1->resize(ui.widget_3->size().width(),ui.widget_3->size().height());


  qcp1->axisRect()->addAxis(QCPAxis::atRight);
  qcp1->axisRect()->axis(QCPAxis::atRight,0)->setPadding(30);
  qcp1->axisRect()->axis(QCPAxis::atRight,1)->setPadding(30);
  qcp1->axisRect()->axis(QCPAxis::atRight,1)->setVisible(false);

  qcp1->axisRect()->axis(QCPAxis::atRight,0)->setTickLabels(true);
  qcp1->yAxis->setTickLabels(false);
  connect(qcp1->yAxis2,SIGNAL(rangeChanged(QCPRange))
      ,qcp1->axisRect()->axis(QCPAxis::atRight,0),SLOT(setRange(QCPRange)));
  qcp1->yAxis2->setVisible(true);

  m_graph2 = qcp1->addGraph(qcp1->xAxis,qcp1->axisRect()->axis(QCPAxis::atRight,0));
  m_graph2->setName("Flow Rate");
  QPen p2(QColor(0,180,60));
  p2.setWidth(2.5);
  m_graph2->setPen(p2);

  qcp1->legend->setVisible(true);
  qcp1->legend->setBrush(QBrush(QColor(255,255,255,230)));
  qcp1->xAxis->setRange(qcp0->xAxis->range().lower,qcp0->xAxis->range().upper);
  qcp1->xAxis->setTicker(ttx);

  qcp1->axisRect()->axis(QCPAxis::atRight,0)->setRange(-5,60);
  mtag3 = new axis_tag(m_graph2->valueAxis());
  auto gp2 = m_graph2->pen(); gp2.setWidth(1);
  mtag3->setPen(gp2);
}

void qcp_monitor::timer_slot1(){
  //debug_out(std::this_thread::get_id());
  //m_data_frame.rw_lock.lockForRead();
  //float yt = util::get_bmp280_1(
  //    m_data_frame.data+REG_TEMPERATURE_DEC);
  //float pt = util::get_bmp280_1(
  //    m_data_frame.data+REG_PRESSURE_DEC);
  //float pc = util::to_float(m_data_frame.data+REG_MFC_RATE_PV);
  //m_data_frame.rw_lock.unlock();

  //using namespace std::chrono;
  //auto dr = duration_cast<system_clock::duration>(
  //    system_clock::now().time_since_epoch()
  //    -m_start.time_since_epoch()).count();
  //double tt = dr/1.e+6;
  //if (tt==m_cache) return;

  //auto rx = qcp_plots["000"]->xAxis->range();
  //if (m_cache >= rx.lower+0.9*rx.size()){
  //  //auto nr = range_trans00(rx);
  //  auto nr = range_trans01(rx,m_interval);
  //  QSharedPointer<QCPAxisTickerText> ttx(
  //      new QCPAxisTickerText);
  //  for (auto&& x : ticks_adapt(nr,5,&util::format_time))
  //    ttx->addTick(x.first,x.second.c_str());


  //  qcp_plots["000"]->xAxis->setRange(nr.lower,nr.upper);
  //  qcp_plots["000"]->xAxis->setTicker(ttx);
  //  qcp_plots["001"]->xAxis->setRange(nr.lower,nr.upper);
  //  qcp_plots["001"]->xAxis->setTicker(ttx);

  //}
  //m_graph0->addData(tt,yt);
  //m_graph1->addData(tt,pt);
  //m_graph2->addData(tt,pc);
  //m_cache = tt;

  //double g1v = m_graph0->dataMainValue(m_graph0->dataCount()-1);
  //double g2v = m_graph1->dataMainValue(m_graph1->dataCount()-1);
  //double g3v = m_graph2->dataMainValue(m_graph2->dataCount()-1);
  //mtag1->updatePosition(g1v);
  //mtag2->updatePosition(g2v);
  //mtag3->updatePosition(g3v);
  //mtag1->setText(QString::number(g1v,'f',2));
  //mtag2->setText(QString::number(g2v,'f',2));
  //mtag3->setText(QString::number(g3v,'f',2));
  //qcp_plots["000"]->replot();
  //qcp_plots["001"]->replot();

 // std::cout<<dr<<" "<<yt<<" "<<pt<<std::endl;



  
}

void qcp_monitor::start(){
  if (_is_fs){
    m_start = std::chrono::system_clock::now();
    _is_fs = false;
    if (m_graph0->data()->size() != 0) m_graph0->data()->clear();
    if (m_graph1->data()->size() != 0) m_graph1->data()->clear();
    for (auto&& [x,y] : qcp_plots) y->replot();
  }
  _is_read.store(true);
  QSharedPointer<QCPAxisTickerText> ttx( new QCPAxisTickerText);
  for(auto&& x : ticks_adapt(qcp_plots["000"]->xAxis->range(),5,&util::format_time)){
    ttx->addTick(x.first,x.second.c_str()); }
  qcp_plots["000"]->xAxis->setTicker(ttx);
  qcp_plots["001"]->xAxis->setTicker(ttx);

  get();
  mtimer.start();


  ui.pushButton->setEnabled(false);
  ui.pushButton_3->setEnabled(true);
}

void qcp_monitor::stop(){
  _is_read.store(false);
  mtimer.stop();
  ui.pushButton_3->setEnabled(false);
  ui.pushButton->setEnabled(true);
}

qcp_monitor::ticks_t qcp_monitor::ticks_adapt(
      QCPRange range, size_t div
      ,std::function<std::string(std::chrono::system_clock::duration,double)> trans){
  if(div<=1) return ticks_t{};
  ticks_t rt;
  double del = range.size()/div;
  for (double i=range.lower; i<=range.upper; i+=del)
    rt.emplace_back(std::make_pair(i
          ,trans(m_start.time_since_epoch()+std::chrono::milliseconds((int)range.lower),i)));
  return rt;
}

void qcp_monitor::aaa(){
  info_out("aaa0");
}
void qcp_monitor::aaa(QMouseEvent*,bool,QVariant const&,bool*){
  info_out("aaa");
}

void qcp_monitor::get(){
  if (!m_parent->_is_connected){
    return;
  }
  std::thread reader([this](){
      while(_is_read.load()){
        m_data_frame.rw_lock.lockForWrite();
        int ec = modbus_read_registers(m_parent->m_modbus->
            m_modbus_context
            ,0,REG_END,&m_data_frame.data[0]);
        m_data_frame.rw_lock.unlock();
        if (ec != REG_END){
          m_parent->recive_text(1,"modbus read error");
        }
        util::t_msleep(m_interval);
      }
      }
      );
  reader.detach();
}

void qcp_monitor::closeEvent(QCloseEvent*){
  if (_is_read.load()) _is_read.store(false);
  mtimer.stop();
  m_graph0->data()->clear();
  m_graph1->data()->clear();
  ui.pushButton->setEnabled(true);
  ui.pushButton_3->setEnabled(true);

}
