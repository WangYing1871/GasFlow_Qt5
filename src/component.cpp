#include <utility>
#include <thread>
#include <bitset>
#include <ios>

#include "modbus_ifc.h"

#include "util.h"

#include "switch.h"
#include "component.h"
#include "mainwindow.h"

using namespace util;

component::component(){ }
//---------------------------------------------------------------------
forward::forward(QWidget* parent):
  QWidget(parent){
  ui.setupUi(this);
}
//---------------------------------------------------------------------
BMP280::BMP280(QWidget* parent): QWidget(parent){
  ui.setupUi(this);
  this->setGeometry(100,100,500,80);
  ui.lineEdit_2->setEnabled(false); ui.lineEdit->setEnabled(false);
  ui.lineEdit_2->setText("--.------"); ui.lineEdit->setText("--.------");

  connect(ui.pushButton_2,&QAbstractButton::clicked,this,&BMP280::T);
  connect(ui.pushButton,&QAbstractButton::clicked,this,&BMP280::P);
};

void BMP280::T(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  uint16_t view[2];
  int ec = modbus_read_registers(mbs_ctx,REG_TEMPERATURE_DEC,2,view);
  if (ec != 2){ m_parent->recive_text(1,"temperature read failed");
    ui.lineEdit_2->setText(""); return; }
  std::stringstream sstr; sstr<<view[0]<<"."<<view[1];
  ui.lineEdit_2->setText(sstr.str().c_str());
  std::stringstream sstr1; sstr1<<"temperature read ok: "<<sstr.str();
  m_parent->recive_text(0,sstr1.str());

  uint8_t ls; modbus_read_bits(mbs_ctx,COIL_STATUS_LATEST,1,&ls); emit latest("bmp280-t",ls);
}

void BMP280::P(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  uint16_t view[2];
  int ec = modbus_read_registers(mbs_ctx,REG_PRESSURE_DEC,2,view);
  if (ec != 2){ m_parent->recive_text(1,"temperature read failed");
    ui.lineEdit->setText(""); return; }
  std::stringstream sstr; sstr<<view[0]<<"."<<view[1];
  ui.lineEdit->setText(sstr.str().c_str());
  std::stringstream sstr1; sstr1<<"temperature read ok: "<<sstr.str();
  m_parent->recive_text(0,sstr1.str());

  uint8_t ls; modbus_read_bits(mbs_ctx,COIL_STATUS_LATEST,1,&ls); emit latest("bmp280-p",ls);
}

//---------------------------------------------------------------------


AHT20::AHT20(QWidget* parent): QWidget(parent){
  ui.setupUi(this);
  this->setGeometry(100,100,270,50);
  ui.lineEdit_2->setText("--.------");
  connect(ui.pushButton_2,&QAbstractButton::clicked,this,&AHT20::H);
}

void AHT20::H(){
  modbus_t* __attribute__((unused)) mbs_ctx = m_parent->m_modbus->m_modbus_context;
  m_parent->recive_text(1,"Humidity is not Support now");
}


//---------------------------------------------------------------------


Valve::Valve(QWidget* parent): QWidget(parent){
  ui.setupUi(this);
  this->setGeometry(100,100,270,50);
  m_sw = new SwitchButton(this); 
  m_sw->setGeometry(10,10,70,30);
  ui.radioButton_4->setChecked(true);
  connect(ui.radioButton_4,&QAbstractButton::clicked,this,&Valve::Switch0);
  connect(ui.radioButton_3,&QAbstractButton::clicked,this,&Valve::Switch1);
  connect(m_sw,SIGNAL(statusChanged(bool)),this,SLOT(enable(bool)));
}

void Valve::Switch0(){
  if (!_is_enable){ m_parent->recive_text(1,"enable valve please"); return; }
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  if (modbus_write_bit(mbs_ctx,COIL_ONOFF_VALVE ,1)==1){
    m_parent->recive_text(0,"valve_switch set to 1 Ok, gas will dump");
    return; }
  m_parent->recive_text(2,"valve_switch set to 1 Failed, May need redo");
}
void Valve::Switch1(){
  if (!_is_enable){ m_parent->recive_text(1,"enable valve please"); return; }
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  if (modbus_write_bit(mbs_ctx,COIL_ONOFF_VALVE,0)==1){
    m_parent->recive_text(0,"valve_switch set to 0 Ok, gas will recycle");
    return; }
  m_parent->recive_text(2,"valve_switch set to 0 Failed, May need redo");
}

void Valve::enable(bool v){
  if (v==_is_enable) return;
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  int ec = modbus_write_bit(mbs_ctx,COIL_ONOFF_PUMP,v);
  if (ec != 1){
    std::stringstream sstr;
    sstr<<"valve switch to "<<std::boolalpha<<v<<" Failed"<<std::noboolalpha;
    m_parent->recive_text(2,sstr.str()); return;}
  std::stringstream sstr;
  sstr<<"valve switch to "<<std::boolalpha<<v<<" Ok "<<std::noboolalpha;
  m_parent->recive_text(0,sstr.str());
  _is_enable = v; }


//---------------------------------------------------------------------


Pump::Pump(QWidget* parent): QWidget(parent){
  ui.setupUi(this);
  this->setGeometry(100,100,610,90);
  m_sw = new SwitchButton(this);
  m_sw->setGeometry(10,10,61,31);
  connect(ui.lineEdit,SIGNAL(editingFinished())
      ,this,SLOT(sync_slider_le()));
  connect(ui.horizontalSlider,&QSlider::valueChanged,this,&Pump::display_slider);
  connect(ui.horizontalSlider,&QSlider::sliderReleased,this,&Pump::set_pwm);
  connect(ui.pushButton,&QAbstractButton::clicked,this,&Pump::reset);
  connect(ui.pushButton_2,&QAbstractButton::clicked,this,&Pump::update);
  connect(m_sw,SIGNAL(statusChanged(bool)),this,SLOT(enable(bool)));
}

void Pump::set_pwm(){
  uint16_t value = ui.horizontalSlider->value();
  ui.lineEdit->setText(QString::number(ui.horizontalSlider->value()));
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  if (!_is_enable){ m_parent->recive_text(1,"enable pump please"); return; }

  std::stringstream sstr;
  if (modbus_write_register(mbs_ctx,REG_PUMP_SPEED_SV,value)==1){
    sstr<<"set pwm Pump Ok. valve "<<value/(float)m_range[1]<<"%";
    m_parent->recive_text(0,sstr.str().c_str());
    return;
  }
  sstr<<"set pwm Pump failed";
  m_parent->recive_text(2,sstr.str().c_str());
}

void Pump::update(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  uint16_t view[5];
  int ec = modbus_read_registers(mbs_ctx,REG_PUMP_SPEED_SV,5, view);
  if (ec != 5){
    m_parent->recive_text(1,"pump info update failed");
    return; }
  m_range[0] = view[3]; m_range[1] = view[4];
  std::stringstream sstr;
  sstr<<"pump in range ["<<m_range[0]<<","<<
    m_range[1]<<"] ";
  m_parent->recive_text(0,sstr.str());
  
  ui.horizontalSlider->setSingleStep(10);
  ui.horizontalSlider->setPageStep(100);
  ui.horizontalSlider->setMinimum(m_range[0]);
  ui.horizontalSlider->setMaximum(m_range[1]);

  std::stringstream sstr1;
  sstr1<<"["<<m_range[0]<<","<<m_range[1]<<"]";
  ui.label_4->setText(sstr1.str().c_str());
  ui.label->setText(std::to_string(view[2]).c_str());
  ui.label_6->setText(std::to_string(view[1]).c_str());
}

void Pump::reset(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  std::stringstream sstr;
  ui.horizontalSlider->setValue(m_range[0]);
  ui.lineEdit->setText(QString::number(m_range[0]));
  if (!_is_enable){ m_parent->recive_text(1,"enable pump please"); return; }
  if (modbus_write_register(mbs_ctx,REG_PUMP_SPEED_SV,m_range[0])==1){
    m_parent->recive_text(0,"reset pump Ok");
    return; }
  m_parent->recive_text(2,"reset pump Failed"); }

void Pump::sync_slider_le(){
  int value = std::stoi(ui.lineEdit->text().toStdString().c_str());
  std::stringstream sstr;
  if (value<m_range[0] || value>m_range[1]){
    sstr<<"Please set Pump in range ["<<m_range[0]<<","<<m_range[1]<<"]"
      <<", your set: "<<value<<" do nothing ~_~";
    m_parent->recive_text(1,sstr.str().c_str());
    return; }
  ui.horizontalSlider->setValue(value);

  if (!_is_enable){ m_parent->recive_text(1,"enable pump please"); return; }
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  if (modbus_write_register(mbs_ctx,REG_PUMP_SPEED_SV,value)==1){
    sstr<<"set pwm Pump Ok. valve "<<100.f*(double)value/m_range[1]<<"%";
    m_parent->recive_text(0,sstr.str().c_str());
    return;
  }
  sstr<<"set pwm Pump failed";
  m_parent->recive_text(2,sstr.str().c_str());
}

void Pump::enable(bool v){
  if (v==_is_enable) return;
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  int ec = modbus_write_bit(mbs_ctx,COIL_ONOFF_PUMP,v);
  if (ec != 1){
    std::stringstream sstr;
    sstr<<"pump switch to "<<std::boolalpha<<v<<" Failed"<<std::noboolalpha;
    m_parent->recive_text(2,sstr.str()); return;}
  std::stringstream sstr;
  sstr<<"pump switch to "<<std::boolalpha<<v<<" Ok "<<std::noboolalpha;
  m_parent->recive_text(0,sstr.str());
  _is_enable = v; }


//---------------------------------------------------------------------

MFC::MFC(QWidget* parent): QWidget(parent){
  ui.setupUi(this);
  m_sw = new SwitchButton(this);
  m_sw->setGeometry(260,10,70,30);
  connect(ui.pushButton_3,&QAbstractButton::clicked,this,&MFC::current);
  connect(ui.pushButton_8,&QAbstractButton::clicked,this,&MFC::total);
  connect(ui.lineEdit,SIGNAL(editingFinished()),this,SLOT(set_curr()));
  connect(ui.pushButton_10,&QAbstractButton::clicked,this,&MFC::zero_total);
  connect(m_sw,SIGNAL(statusChanged(bool)),this,SLOT(enable(bool)));

  //ui.lineEdit->setText("0.0");
  //set_curr();
}

void MFC::current(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  std::stringstream sstr;
  if (!_is_enable){ m_parent->recive_text(1,"enable MFC please"); return; }
  uint16_t view[2];
  int ec = modbus_read_registers(mbs_ctx,REG_MFC_RATE_PV,2,view);
  if (ec != 2){
    m_parent->recive_text(1,"read MFC flow failed");
    return; }
  uint32_t v10 = std::bitset<32>{std::bitset<16>{view[1]}.to_string()+
    std::bitset<16>{view[0]}.to_string()}.to_ulong();
  float* curr_f = reinterpret_cast<float*>(&v10);
  sstr<<*curr_f<<" mL/min";
  ui.label_11->setText(sstr.str().c_str());
  sstr.clear();
  sstr<<"(MFC) current flow:  "<<ui.label_11->text().toStdString();
  m_parent->recive_text(0,sstr.str());
}

void MFC::total(){
  //modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  //std::stringstream sstr;
  //if(modbus_write_register(mbs_ctx,9,1)!=1){
  //  sstr<<"ask for MFC total (write addr 9(1)) command Failed.";
  //  m_parent->recive_text(2,sstr.str());
  //  return;
  //}
  //std::this_thread::sleep_for(std::chrono::milliseconds(800));
  //auto* view = new uint16_t[16];
  //if (modbus_read_registers(mbs_ctx,12,2,view)!=2){
  //  sstr<<"Read for MFC total(read addr 10,11) data Failed.";
  //  m_parent->recive_text(2,sstr.str());
  //  return;
  //}
  //uint32_t v10 = std::bitset<32>{std::bitset<16>{view[1]}.to_string()+
  //  std::bitset<16>{view[0]}.to_string()}.to_ulong();
  //float* curr_f = reinterpret_cast<float*>(&v10);
  //sstr<<*curr_f<<" mL/min";
  //ui.label_13->setText(sstr.str().c_str());
  //sstr.clear();
  //sstr<<"MFC total flow is "<<ui.label_13->text().toStdString();
  //m_parent->recive_text(0,sstr.str());
}

void MFC::set_curr(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  std::string text_str = ui.lineEdit->text().toStdString();
  float value = std::stof(text_str.c_str());
  std::stringstream sstr;
  uint16_t* addr = reinterpret_cast<uint16_t*>(&value);
  uint16_t zero[2] = {addr[0],addr[1]};
  sstr.clear();
  if (!_is_enable){ m_parent->recive_text(1,"enable MFC please"); return; }
  if (modbus_write_registers(mbs_ctx,REG_MFC_RATE_SV,2,zero)!=2){
    sstr<<"MFC set to "<<value<<" Failed" << "modbus_strerror(error): "<<modbus_strerror(errno);
    m_parent->recive_text(2,sstr.str().c_str());
    return;
  }
  sstr<<"MFC set to "<<value<<" Ok";
  m_parent->recive_text(0,sstr.str().c_str());
  uint8_t ls; modbus_read_bits(mbs_ctx,COIL_STATUS_LATEST,1,&ls);
  std::stringstream text; text<<"Latest status "<<ls;
  m_parent->recive_text(1,text.str());
}


void MFC::zero_total(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  if (modbus_write_register(mbs_ctx,17,1)!=1){
    m_parent->recive_text(2,"Reset(to zero) total flow Failed");
    return;
  }
  m_parent->recive_text(0,"Reset(to zero) total flow Ok");
}

void MFC::closeEvent(QCloseEvent* event){
  //if (m_parent->_is_debug){
  //  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  //  uint16_t zero[2] = {0,0};
  //  modbus_write_registers(mbs_ctx,REG_MFC_RATE_SV,2,zero);
  //}
}

void MFC::enable(bool v){
  if (v==_is_enable) return;
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  int ec = modbus_write_bit(mbs_ctx,COIL_ONOFF_MFC,v);
  if (ec != 1){
    std::stringstream sstr;
    sstr<<"MFC switch to"<<std::boolalpha<<v<<" Failed"<<std::noboolalpha;
    m_parent->recive_text(2,sstr.str()); return;}
  std::stringstream sstr;
  sstr<<"MFC switch to"<<std::boolalpha<<v<<" Ok"<<std::noboolalpha;
  m_parent->recive_text(0,sstr.str());
  _is_enable = v; }

//---------------------------------------------------------------------

Modbus_setting::Modbus_setting(QWidget* parent): QDialog(parent){
  ui.setupUi(this);
  connect(ui.applyButton,&QAbstractButton::clicked,this,&Modbus_setting::set);

   
 // connect(this,SIGNAL(send_text()), m_parent,SLOT(recive_text()));


  
}

void Modbus_setting::set(){
  std::string parity_str = ui.parityCombo->currentText().toStdString();
  if (parity_str=="No") m_params.parity='N';
  else if(parity_str=="Even") m_params.parity='E';
  else if(parity_str=="Odd") m_params.parity='O';
  else{
    std::stringstream sstr; sstr<<"Modbus_setting parity to '"<<
      parity_str<<"' not support now. Set to No";
    m_parent->recive_text(1,sstr.str());
    m_parent->ui.textEdit->update();
    m_params.parity='N'; }

  m_params.baud = std::stoi(ui.baudCombo->currentText().toStdString().c_str());
  m_params.data_bit = std::stoi(ui.dataBitsCombo->currentText().toStdString());
  m_params.stop_bit= std::stoi(ui.stopBitsCombo->currentText().toStdString());
  std::stringstream sstr; sstr<<"Set Serial parameters"
    <<"\n  baud: "<<m_params.baud
    <<"\n  parit: "<<m_params.parity
    <<"\n  data_bit: "<<m_params.data_bit
    <<"\n  stop_bit: "<<m_params.stop_bit;
  m_parent->recive_text(0,sstr.str());
  this->close();
}
//---------------------------------------------------------------------
int Modbus_setting::read_register(uint16_t pos, uint16_t count, uint16_t* dest){
  //std::stringstream sstr; sstr<<"pos: "<<pos<<", count: "<<count;
  //m_parent->recive_text(0,sstr.str(),1);
  if (!link())
    m_parent->recive_text(2,"May be shoule link first",1);
  return modbus_read_registers(m_modbus_context,pos,count,dest);
}
