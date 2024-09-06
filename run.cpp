#include <QMessageBox>
#include "run.h"
#include "mainwindow.h"

run::run(QWidget* parent):QWidget(parent){
  std::cout<<"wangying"<<std::endl;
  ui.setupUi(this);
  //m_parent->recive_text(0,"Run Mode");
  connect(ui.radioButton_2,&QAbstractButton::clicked,this,&run::recycle_mode);
  connect(ui.radioButton,&QAbstractButton::clicked,this,&run::dump_mode);
  connect(ui.radioButton_3,&QAbstractButton::clicked,this,&run::self_cycle_mode);
  m_parent->_is_debug = false;
}

void run::recycle_mode(){
  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;

  if (modbus_write_register(mbs_ctx,24,0)!=1){
    m_parent->recive_text(2,"Run: recycle_mode Failed");
    return;
  }
  m_parent->recive_text(0,"Run: recycle_mode OK");

}

void run::dump_mode(){
  std::string text_str = ui.lineEdit->text().toStdString();
  float value = 0.;
  try{
    value = std::stof(text_str.c_str());
  }catch(std::invalid_argument& e){
    m_parent->recive_text(1,"[EX invalid_argument]: invalid input in LineEdit. IEEE float needed. set to 20.f as default");
    value = 20.;
  }
  std::stringstream sstr;
  uint16_t* addr = reinterpret_cast<uint16_t*>(&value);
  uint16_t zero[3] = {addr[0],addr[1],1};

  modbus_t* mbs_ctx = m_parent->m_modbus->m_modbus_context;
  if(modbus_write_registers(mbs_ctx,22,3,zero)!=3){
    m_parent->recive_text(2,"Run: dump mode Failed");
    return; }
  m_parent->recive_text(0,"Run: dump mode Ok");
}

void run::self_cycle_mode(){
  std::string text_str = ui.lineEdit->text().toStdString();


}

//void run::show_all_monitor(QSize pos){
//  for (auto&& x : status_minitors){
//    QSize ws = x->size();
//    x->setGeometry(pos.width(),pos.height(),ws.width(),ws.height());
//    x->show(); x->update();
//    std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
//    ruler += (40+ws.height());
//    x->setEnabled(false);
//  }
//}
void run::closeEvent(QCloseEvent* event){
  event->ignore();
  //for (auto&& x : m_childs) x->close();
  m_parent->setEnabled(true);
  this->close();
}

