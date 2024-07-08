#include <stdio.h>
#include <errno.h>
#include <dirent.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>
#include <map>
#include <ctime>
#include <tuple>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <bitset>
#include <type_traits>

//#include <boost/filesystem.hpp>
//namespace bst_ft = boost::filesystem;

#include <QtWidgets>
#include <QDialog>
#include <QMenuBar>
#include <QAction>
#include <QMenu>
#include <QDir>
#include <QValueAxis>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QThread>
#include <QFile>

#include <QtCharts/QChartView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QGraphicsLayout>

#include "modbus/modbus.h"
#include "mainwindow.h"
#include "form.h"
#include "run.h"
#include "util.h"
#include "modbus_ifc.h"
//---------------------------------------------------------------------
mainwindow::mainwindow():
  QMainWindow(){
  ui.setupUi(this);

  ui.groupBox_6->setStyleSheet(form::group_box_font0);
  m_modbus = new Modbus_setting();
  m_modbus->parent(this);
  connect(ui.action_serial,&QAction::triggered,m_modbus,&QDialog::show);

  connect(ui.action_About,&QAction::triggered,this,&mainwindow::About);
  connect(ui.action_About_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  connect(ui.action_exit,&QAction::triggered,this,&mainwindow::exit);

  ui.comboBox_13->setCurrentIndex(0);
  connect(ui.comboBox_13,SIGNAL(currentIndexChanged(int)),this,SLOT(modbus_mode(int)));

  fill_ports_info(); 
  QString cs = ui.comboBox_12->currentText();
  info_out(cs.toStdString());

  if (ui.comboBox_13->currentText().toStdString()=="RTU"){
    ui.spinBox_7->setMinimum(1); ui.spinBox_7->setMaximum(255);
    ui.spinBox_7->setValue(1); ui.spinBox_7->setSingleStep(1);
    ui.spinBox_7->setStepType(QAbstractSpinBox::DefaultStepType); }

  connect(ui.pushButton_2,&QAbstractButton::clicked,this,&mainwindow::browser);
  if(!(cs=="custom")) 
    ui.pushButton_2->setEnabled(false);
  connect(ui.comboBox_12,SIGNAL(currentIndexChanged(int))
      ,this
      ,SLOT(enable_browser(int)));

  std::string dc_icon_name = util::pwd()+"/images/disconnect.png";
  ui.toolButton_5->setIcon(QIcon{dc_icon_name.c_str()});
  connect(ui.toolButton_5,&QAbstractButton::clicked,this,&mainwindow::modbus_connect_rtu);

  std::array<QString,32> reg_count;
  std::cout<<ui.comboBox_3<<" "<<ui.comboBox_5<<std::endl;
  for (int i=0; i<32; ++i) reg_count[i] = QString(std::to_string(i+1).c_str());
  for (auto&& x : reg_count) ui.comboBox_3->addItem(x);
  for (auto&& x : reg_count) ui.comboBox_5->addItem(x);

  connect(ui.comboBox_3,SIGNAL(currentIndexChanged(int)),this,SLOT(Modbus_read_registers(int)));

  m_comps.insert("Temperature",new BMP280())
    ->insert("Humidity",new AHT20())
    ->insert("Valve-Switch",new Valve())
    ->insert("Pump",new Pump())
    ->insert("Micro-Flow-Control",new MFC())
    ;
  dynamic_cast<BMP280*>(m_comps.at("Temperature"))->parent(this);
  dynamic_cast<AHT20*>(m_comps.at("Humidity"))->parent(this);
  dynamic_cast<Valve*>(m_comps.at("Valve-Switch"))->parent(this);
  dynamic_cast<Pump*>(m_comps.at("Pump"))->parent(this);
  dynamic_cast<MFC*>(m_comps.at("Micro-Flow-Control"))->parent(this);

  connect(
      dynamic_cast<BMP280*>(m_comps.at("Temperature"))
      ,SIGNAL(latest(std::string const&,bool))
      ,this
      ,SLOT(update_statusbar(std::string const&,bool))
      );


  //connect(ui.action_run,&QAction::triggered,this,&mainwindow::Run);
  connect(ui.action_connect,&QAction::triggered,this,&mainwindow::modbus_connect_rtu);
  connect(ui.action_disconnect,&QAction::triggered,this,&mainwindow::modbus_disconnect_rtu);
  //auto* widget_bmp280 = new BMP280();
  connect(ui.actionBMP280,&QAction::triggered,m_comps.at("Temperature"),&QWidget::show);

  ////auto* widget_aht20 = new AHT20();
  connect(ui.actionAHT20,&QAction::triggered,m_comps.at("Humidity"),&QWidget::show);

  ////auto* widget_valve_0 = new Valve();
  connect(ui.action_Valve,&QAction::triggered,m_comps.at("Valve-Switch"),&QWidget::show);
  

  ////auto* widget_pwm_0 = new Pump();
  connect(ui.actionPump,&QAction::triggered,this,&mainwindow::show_pump);

  ////auto* widget_MFC_0 = new MFC();
  connect(ui.action_MCF,&QAction::triggered,m_comps.at("Micro-Flow-Control"),&QWidget::show);


  //m_runer = new run(); 
  //m_runer->parent(this);

  
  m_monitors = new monitors(this,nullptr);
  connect(ui.actionmonitor,&QAction::triggered,this,&mainwindow::show_monitor);

  //m_monitors_qcp = new qcp_monitor(this,nullptr);
  //connect(ui.actionmonitor,&QAction::triggered,this,&mainwindow::show_monitor_qcp);

  //m_pwb = new pwb();
  //m_pwb->parent(this); 
  //m_pwb->m_modbus = m_modbus->m_modbus_context;
  //connect(ui.actionpwb,&QAction::triggered,this,&mainwindow::show_pwb);

  m_pwb_v1 = new pwb_v1();
  m_pwb_v1->parent(this);
  m_pwb_v1->modbus_context(m_modbus->m_modbus_context);
  connect(ui.actionpwb,&QAction::triggered,this,&mainwindow::show_pwb);

  //init_monitors();

  //for(auto&& x : m_historys) x = new QWidget, x->setFixedSize(16,16);
  //for (auto&& x : m_historys) ui.statusbar->addWidget(x);
  //for (auto&& x : m_historys) x->setStyleSheet("background: grey;");

}

void mainwindow::update_statusbar(std::string const& v,bool s){
  //m_history_record.emplace_back(s);
  //auto led = new QWidget; led->setFixedSize(16,16);
  //
  //m_historys.emplace_back(new QWidget*)
  


}

void mainwindow::show_pump(){
  if (!_is_connected) { this->recive_text(2,"please connect serial port"); return; }
  uint16_t view[5]; auto* ptr = dynamic_cast<Pump*>(m_comps.at("Pump"));
  ptr->update(); ptr->show(); }

void mainwindow::show_pwb(){
  m_pwb_v1->show();
  //m_pwb->m_timer.start();
}
void mainwindow::show_monitor(){
  if (_is_connected){
    auto* modbus_ctx = m_modbus->m_modbus_context;
    int ec0 = modbus_read_registers(modbus_ctx,0,REG_END,m_monitors->m_data_frame.data);
    util::t_msleep(20);
    int ec1 = modbus_read_bits(modbus_ctx,0,COIL_END,m_monitors->m_data_frame.device_coil);
    if (ec0!=REG_END || ec1!=COIL_END){
      this->recive_text(1,"Monitors update firmwire status Failed"); return; }
    m_monitors->update_mode();
    m_monitors->update_forward(); 
    update_device();
    //this->setEnabled(false);
  }
  else{
  }
  //update_device();
  m_monitors->show();
}

void mainwindow::update_device(){
  m_device_init.clear();
  for (int i=COIL_STATUS_MFC; i<COIL_STATUS_LATEST; ++i){
    auto* tmp = new QWidget; tmp->setFixedSize(16,16); ui.statusbar->addWidget(tmp);
    tmp->setStyleSheet(util::state_ss(m_monitors->m_data_frame.device_coil[i]).c_str());
    m_device_init.emplace_back(tmp); }

  //for (int i=0; i<5; ++i){
  //  auto* tmp =new QWidget; tmp->setFixedSize(16,16);
  //  ui.statusbar->addWidget(tmp);
  //  QFile styleSheet(":/files/nature_1.jpg");
  //  if (!styleSheet.open(QIODevice::ReadOnly)) {
  //    qWarning("Unable to open :/files/nostyle.qss"); return; }
  //}
  m_newest = new QLabel;
  ui.statusbar->addWidget(m_newest);
  m_newest->setText("unknow");


}

bool mainwindow::is_connect(){
  if(!m_modbus || !_is_connected){
    std::string info = "please connect serial device";
    util::info_to<QTextEdit,util::log_mode::k_warning>(this->ui.textEdit,info);
    return false; }
  if (modbus_connect(m_modbus->m_modbus_context)==-1){
    std::string info = "modbus connect failed";
    util::info_to<QTextEdit,util::log_mode::k_warning>(this->ui.textEdit,info);
    return false; }
  return true; }
//---------------------------------------------------------------------
//void mainwindow::init_monitors(){
//  QString names[] = { "temperature" ,"pressure" ,"MFC-PV" };
//  for (auto&& x : names)
//    m_monitors[x.toStdString().c_str()] = new monitor;
//  for (auto&& [_,x] : m_monitors)
//    x->setTitle(_.c_str()), x->legend()->hide(),
//    x->setAnimationOptions(QChart::AllAnimations),
//    x->setMargins(QMargins(0,0,0,0)),
//    x->layout()->setContentsMargins(0,0,0,0);
//  //for (auto&& [_,x] : m_monitors) x->format();
//  for (auto&& [_,x] : m_monitors) x->format_line();
//}

//---------------------------------------------------------------------
void mainwindow::recive_text(size_t mode, std::string const& value, int pos){
  typedef decltype(ui.textEdit) text_t;
  text_t active = 
    pos==0 ? ui.textEdit : pos==1 ? ui.textEdit_2 : pos==2 ? ui.textEdit_3 : ui.textEdit;
  //std::cout<<"wangying"<<std::endl;
  mode==0 ?  util::info_to<QTextEdit,util::log_mode::k_info>(active,value) : 
    mode==1 ?  util::info_to<QTextEdit,util::log_mode::k_warning>(active,value) : 
      mode==2 ?  util::info_to<QTextEdit,util::log_mode::k_error>(active,value) :
        util::info_to<QTextEdit,util::log_mode::k_error>(active,"unknow content mode, display as ERROR!");
}
//---------------------------------------------------------------------
void mainwindow::exit(){
  for (auto&& x : m_comps.m_components){
    if(x.second->isVisible()) x.second->close();
  }
  //auto* window = m_comps.m_components.at("Temperature");
  //for (int i=0; i<10; ++i){
  //  window->setGeometry(0+30*i,0+10*i,270,80);
  //  window->show();
  //  std::this_thread::sleep_for(std::chrono::mil/liseconds(10000000));
  //  window->close();
  //}
  if (this->isVisible()) this->close();
}

void mainwindow::show_monitor_qcp(){
  //m_monitors_qcp->show();
}

void mainwindow::modbus_mode(int __attribute__((unused)) mode){
  std::string modbus_name = ui.comboBox_13->currentText().toStdString();
  if (modbus_name=="RTU"){
    //bst_ft::path dev("/dev");
    //ui.comboBox_2->clear();
    //for (auto iter = bst_ft::directory_iterator( dev); iter!=bst_ft::directory_iterator{}; ++iter){
    //  std::string name = iter->path().string();
    //  if (name.find("USB") != std::string::npos) ui.comboBox_2->addItem(QString{name.c_str()}); }
    ui.spinBox_7->setMinimum(1);
    ui.spinBox_7->setMaximum(255);
    ui.spinBox_7->setValue(1);
    ui.spinBox_7->setSingleStep(1);
    ui.spinBox_7->setStepType(QAbstractSpinBox::DefaultStepType);
  }else{
    ui.comboBox_12->clear();
    util::info_to<QTextEdit,util::log_mode::k_warning>(ui.textEdit,"modbus-mode TCP Not Support!");
    ui.spinBox_7->setMinimum(0);
    ui.spinBox_7->setMaximum(255);
    ui.spinBox_7->setValue(0);
    ui.spinBox_7->setSingleStep(1);
    ui.spinBox_7->setStepType(QAbstractSpinBox::DefaultStepType);
  }
}

void mainwindow::modbus_connect_rtu(){
  std::string device = ui.comboBox_12->currentText().toStdString();
  int slave_addr = ui.spinBox_7->value();
  auto& p = m_modbus->modbus_parameters();

  auto cs = util::connect_modbus(device.c_str(),p.baud,p.parity,p.data_bit,p.stop_bit,slave_addr);
  std::cout<<cs.first<<" "<<cs.second<<std::endl;
  if (cs.second){
    std::string name = util::pwd()+"/images/connect.png";
    ui.toolButton_5->setIcon(QIcon(name.c_str()));
    m_modbus->m_modbus_context = cs.first;
    m_modbus->link(true);
    util::info_to<QTextEdit,util::log_mode::k_info>(ui.textEdit,
        "Link modbus device Ok.", " device: ", device
        ,"baud: ",p.baud
        ,"parity: ",p.parity
        ,"data_bit: ",p.data_bit
        ,"stop_bit: ",p.stop_bit
        ,"slave_addr: ",slave_addr
        );
    _is_connected = true;
  }else{
    m_modbus->link(false);
    modbus_close(m_modbus->m_modbus_context); modbus_free(m_modbus->m_modbus_context);
    util::info_to<QTextEdit,util::log_mode::k_error>(ui.textEdit,
        "Link modbus device Failed.", " device: ", device
        ,"baud: ",p.baud
        ,"parity: ",p.parity
        ,"data_bit: ",p.data_bit
        ,"stop_bit: ",p.stop_bit
        ,"slave_addr: ",slave_addr
        ,"Chcek PLease");
    _is_connected = false;
  }
}
void mainwindow::modbus_disconnect_rtu(){
  modbus_close(m_modbus->m_modbus_context); modbus_free(m_modbus->m_modbus_context);
  util::info_to<QTextEdit,util::log_mode::k_info>(ui.textEdit,"disconnect modbus serial, Bye");
  ui.toolButton_5->setIcon(QIcon(":/images/disconnect.png"));
  m_modbus->link(false);
  _is_connected = false;
}

void mainwindow::closeEvent(QCloseEvent* e){
  (void)e;
  //auto const& maybesave = [this](){
  //  auto ret = QMessageBox::warning(this
  //      ,tr("wangying")
  //      ,tr("Wangying")
  //      ,QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  //  return ret; };
  //maybesave();
  util::info_to<QTextEdit,util::log_mode::k_info>(ui.textEdit,"Exit!");
  for (auto&& x : m_comps.m_components)
    if (x.second->isVisible()) x.second->close();
  //if (m_runer->isVisible()) m_runer->close();
  modbus_disconnect_rtu(); }

//---------------------------------------------------------------------
void mainwindow::Modbus_read_registers(int __attribute__((unused)) count){
  int count_number = std::stoi(ui.comboBox_3->currentText().toStdString().c_str());
  uint16_t* data = new uint16_t[count_number];
    int __attribute__((unused)) ec = m_modbus->read_register(
        ui.spinBox_2->value()
        ,std::stoi(ui.comboBox_3->currentText().toStdString().c_str())
        ,data
        );
    std::stringstream sstr; sstr<<count_number<<", value:"<<util::to_float(data);
    //util::info_to<QTextEdit,util::log_mode::k_info>(ui.textEdit,sstr.str(),1);
    this->recive_text(0,sstr.str(),1);
    //util::t_msleep(1000);
  //}


  delete[] data;
}
//---------------------------------------------------------------------
void mainwindow::browser(){
  QString const dev = QFileDialog::getOpenFileName(this);
  ui.comboBox_12->addItem(dev);
}

void mainwindow::fill_ports_info(){
  ui.comboBox_12->clear();
  auto const infos = QSerialPortInfo::availablePorts();
  QString description;
  QString manufacturer;
  QString serialNumber;
  for (auto&& x : infos){
    QStringList list;
    description = x.description();
    manufacturer = x.manufacturer();
    serialNumber = x.serialNumber();
    list << x.portName()
      <<(!description.isEmpty() ? description : util::blank_str)
      <<(!manufacturer.isEmpty() ? manufacturer : util::blank_str)
      << x.systemLocation()
      << (x.vendorIdentifier() ? QString::number(x.vendorIdentifier(), 16) : util::blank_str)
      << (x.productIdentifier() ? QString::number(x.productIdentifier(), 16) : util::blank_str);
#ifdef WIN32
    ui.comboBox_12->addItem(list.first(),list);
#else
    std::stringstream sstr;
    sstr<<"/dev/"<<list.first().toStdString().c_str();
    ui.comboBox_12->addItem(sstr.str().c_str());
#endif
  }
  ui.comboBox_12->addItem("custom");
  //ui.comboBox_12

}
//---------------------------------------------------------------------
void mainwindow::enable_browser(int){
  std::string name = ui.comboBox_12->currentText().toStdString();
  if (name=="custom") ui.pushButton_2->setEnabled(true);
  else ui.pushButton_2->setEnabled(false);
  //if (name=="custom") ui.pushButton_2->setEnabled(true);
}

void mainwindow::Monitor(){
#ifdef D_NO_SERIAL
  if (_is_thread_read){
    util::info_to<QTextEdit,util::log_mode::k_warning>(ui.textEdit,"already start");
    return; }
  this->setEnabled(false);
  _is_thread_read = true;
  std::thread connect([this](){
      
    std::ifstream fin("130582283607523_default.csv");
    std::string str_buf; std::getline(fin,str_buf);

    std::vector<std::vector<bool>> aa = 
    {
    {1,1,0}
    ,{1,0,0}
    ,{0,1,0}
    ,{0,0,1}
    ,{1,1,1}
    };
    uint16_t index;
    while(_is_thread_read){
      std::string s0, s1;
      std::getline(fin,s0); std::getline(fin,s1);
      for (auto&& x : s0) if(x==',') x=' ';
      for (auto&& x : s1) if(x==',') x=' ';
      std::stringstream sstr0(s0.c_str()), sstr1(s1.c_str());
      for (int i=0; i<23; ++i) m_monitors->m_data_frame.data[i] = index+i;
      for (int i=0; i<3; ++i) m_monitors->m_data_frame.device_coil[i] = aa[index%5][i];
      index++;
      util::t_msleep(400);
    }
    //pthread_rwlock_unlock(&this->m_monitors->m_data_frame.rw_lock);
  });
  connect.detach();
  _is_moniting = true;


#else
  if (!_is_connected){
    util::info_to<QTextEdit,util::log_mode::k_warning>(ui.textEdit,"not connect yet");
    return; }
  if (_is_thread_read){
    util::info_to<QTextEdit,util::log_mode::k_warning>(ui.textEdit,"already start");
    return; }
  _is_thread_read = true;
  std::thread connect([this](){
      
    int ec __attribute__((unused))=0;
    auto* modbus_ctx = this->m_modbus->m_modbus_context;
    uint16_t* addr = std::addressof(this->m_monitors->m_data_frame.data[0]);
    uint8_t* addr_coil = std::addressof(this->m_monitors->m_data_frame.device_coil[0]);
    std::ofstream& fout = m_monitors->m_fout;
    fout<<"# Record time: "<<util::highrestime_ns()<<"\n# timestamp,  ";
    size_t as = std::extent<decltype(util::item_names)>::value;
    for (size_t i=0; i<as-1; ++i) fout<<util::item_names[i];
    fout<<util::item_names[as-1]<<"\n";
    //fout<<"# MFC_coil\tPump_coil\tValve_coil\n";

    size_t index = 0;
    while(_is_thread_read){
      if (index++%20==0) fout.flush();

      this->m_monitors->m_data_frame.tag = util::highrestime_ns();
      int ec = modbus_read_registers(modbus_ctx,0,REG_END,addr);
      if (ec != REG_END)
        util::info_to<QTextEdit,util::log_mode::k_warning>(this->ui.textEdit
              ,std::this_thread::get_id() ,"read registers error catched");
      else{
        fout<<this->m_monitors->m_data_frame.tag<<", ";
        for (int i=0; i<REG_END-1; ++i) fout<<addr[i]<<", ";
        fout<<addr[REG_END-1]<<"\n";
      }
      util::t_msleep(30);
      ec = modbus_read_bits(modbus_ctx,0,3,addr_coil);
      if (ec != 3)
        util::info_to<QTextEdit,util::log_mode::k_warning>(this->ui.textEdit
              ,std::this_thread::get_id() ,"read coils error catched");
      //else fout <<(int)addr_coil[0]<<" "<<(int)addr_coil[1]<<" " <<(int)addr_coil[2]<<"\n";
      util::t_msleep(950);

    }
    //pthread_rwlock_unlock(&this->m_monitors->m_data_frame.rw_lock);
    fout.flush(); 
  });
  connect.detach();
  _is_moniting = true;
#endif
}
void mainwindow::About(){
  QMessageBox::about(this, tr("Gas Flow Control"),
          tr("<h2>GasFlow GUI 0.0.1</h2>"
             "<p>Copyright &copy; 2024 Software Inc. JW company."
             "<p>GasFlow is a small application that use for Micro-Gas-Control"
             "<p>Author(s): Wang Ying; Zhou Yong"
            ));
}

