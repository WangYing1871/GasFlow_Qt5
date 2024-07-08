#include <iostream>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <stdexcept>
#include "mainwindow.h"
#include "pwb.h"

#include "util.h"

pwb::pwb(QWidget* p):
  QWidget(p){
  this->setObjectName("PWB");
  ui.setupUi(this);

  ui.lineEdit->setEnabled(false);
  ui.lineEdit_8->setEnabled(false);
  ui.lineEdit_9->setEnabled(false);
  ui.lineEdit_10->setEnabled(false);
  ui.lineEdit_11->setEnabled(false);

  connect(ui.toolButton_5,&QAbstractButton::clicked,this,&pwb::enable_c0);
  connect(ui.toolButton_6,&QAbstractButton::clicked,this,&pwb::enable_c1);
  connect(ui.toolButton_7,&QAbstractButton::clicked,this,&pwb::enable_c2);
  connect(ui.toolButton_8,&QAbstractButton::clicked,this,&pwb::enable_c3);

  //QWidget::setStyleSheet("QToolButton#ui.toolButton_5 {" " color: white;"
  QWidget::setStyleSheet("QToolButton#toolButton {" " color: white;"
    " border-radius: 20px;" " border-style: solid;"
    " border-color: black;" " border-width: 2px;"
    " background-color: blue;" "}");
  ui.toolButton_5->setStyleSheet("background-color: transparent;");
  ui.toolButton_6->setStyleSheet("background-color: transparent;");
  ui.toolButton_7->setStyleSheet("background-color: transparent;");
  ui.toolButton_8->setStyleSheet("background-color: transparent;");
  ui.toolButton_5->setIcon(QIcon("images/rb.png"));
  ui.toolButton_6->setIcon(QIcon("images/rb.png"));
  ui.toolButton_7->setIcon(QIcon("images/rb.png"));
  ui.toolButton_8->setIcon(QIcon("images/rb.png"));

  decltype(ui.lineEdit_2) les[] = {
    ui.lineEdit_2 ,ui.lineEdit_3 ,ui.lineEdit_4 ,ui.lineEdit_5
    ,ui.lineEdit_8 ,ui.lineEdit_9 ,ui.lineEdit_10 ,ui.lineEdit_11 };
  for (auto&& x : les) x->setAlignment(Qt::AlignJustify);


  connect(ui.pushButton_4,&QAbstractButton::clicked,this,&pwb::stop);
  connect(ui.pushButton,&QAbstractButton::clicked,this,&pwb::update);
  connect(ui.horizontalSlider_5,&QSlider::sliderReleased,this,&pwb::set_c0);
  connect(this,&pwb::aaa,this,&pwb::set_c0);
  //connect(ui.horizontalSlider_6,&QSlider::sliderReleased,this,&pwb::set_c1);
  //connect(ui.horizontalSlider_7,&QSlider::sliderReleased,this,&pwb::set_c2);
  //connect(ui.horizontalSlider_8,&QSlider::sliderReleased,this,&pwb::set_c3);
  //connect(ui.horizontalSlider_5,&QSlider::valueChanged,this,&pwb::set_c0);
  std::cout<<"pwb m_modbus: "<<m_modbus<<std::endl;

  connect(ui.lineEdit_2,SIGNAL(editingFinished()),this,SLOT(set_c0_adpator()));
  
  connect(&m_timer,&QTimer::timeout,this,&pwb::count);
  m_timer.setInterval(1000); }

void pwb::count(){
  auto* md_ctx = m_parent->m_modbus->m_modbus_context;
  if (!m_parent->is_connect()) return;
  uint16_t ct[2];
  int ec = modbus_read_registers(md_ctx,19,2,ct);
  if (ec != 2){m_parent->recive_text(1,"update time failed"); return;}
  uint32_t ct_32 = *reinterpret_cast<uint32_t*>(ct);
  ui.lineEdit->setText(std::to_string(ct_32).c_str());

  uint16_t view[8];
  ec = modbus_read_registers(md_ctx,4,8,view);
  if (ec != 16){m_parent->recive_text(1,"update PI or PV failed"); return;}
  //for (auto&& x : view) std::cout<<std::hex<<(int)x<<" ";
  std::stringstream sstr; 
  sstr.clear(); sstr<<std::hex<<view[0]; ui.label_13->setText(sstr.str().c_str());
  sstr.clear(); sstr<<std::hex<<view[1]; ui.label_18->setText(sstr.str().c_str());
  sstr.clear(); sstr<<std::hex<<view[2]; ui.label_20->setText(sstr.str().c_str());
  sstr.clear(); sstr<<std::hex<<view[3]; ui.label_22->setText(sstr.str().c_str());
  sstr.clear(); sstr<<std::hex<<view[4]; ui.label_14->setText(sstr.str().c_str());
  sstr.clear(); sstr<<std::hex<<view[5]; ui.label_19->setText(sstr.str().c_str());
  sstr.clear(); sstr<<std::hex<<view[6]; ui.label_21->setText(sstr.str().c_str());
  sstr.clear(); sstr<<std::hex<<view[7]; ui.label_23->setText(sstr.str().c_str()); }
//---------------------------------------------------------------------
#define function_coils_register(NAME,INEDX,BUTTON)                      \
void pwb::NAME(){                                                       \
  if (!m_parent->is_connect()) return;                                  \
  auto* md_ctx = m_parent->m_modbus->m_modbus_context;                  \
  int ec = 0; const uint8_t index = INEDX;                              \
  if(util::bit(m_coils,index)==1){                                      \
    ec = modbus_write_bit(md_ctx,index,false);                          \
    if (ec != 1){                                                       \
      std::stringstream text; text<<"set coil "<<index<<" '0' failed";  \
      m_parent->recive_text(1,text.str());                              \
      return; }                                                         \
    ui.BUTTON->setIcon(QIcon{"images/rb.png"});                         \
    util::bit(m_coils,index,0);}                                        \
  else{                                                                 \
    ec = modbus_write_bit(md_ctx,index,true);                           \
    if (ec != 1){                                                       \
      std::stringstream text; text<<"set coil "<<index<<" '1' failed";  \
      m_parent->recive_text(1,text.str());                              \
      return; }                                                         \
    ui.BUTTON->setIcon(QIcon{"images/gb.png"});                         \
    util::bit(m_coils,index,1); } }                                     \
/**/
function_coils_register(enable_c0,4,toolButton_5)
function_coils_register(enable_c1,5,toolButton_6)
function_coils_register(enable_c2,6,toolButton_7)
function_coils_register(enable_c3,7,toolButton_8)
//---------------------------------------------------------------------
//#define function_registers_register(NAME,RINDEX,CINDEX,H,L)
//void pwb::NAME(){
//  if (!m_parent->is_connect()) return;
//  if (util::bit(m_coils,CINDEX)==0){
//    std::stringstream text; text<<"enable channel "<<RINDEX<<" please";
//    m_parent->recive_text(1,text.str()); return; }
//  auto* md_ctx = m_parent->m_modbus->m_modbus_context;
//  uint16_t value = ui.horizontalSlider_5->value();
//  constexpr uint16_t const max_v = 0xFFFF;
//  float dac = 2.5f*value/max_v;
//  ui.lineEdit_2->setText(std::to_string(dac).substr(0,5).c_str());
//  uint8_t const index = 0;
//  int ec = modbus_write_register(md_ctx,index,value);
//  if (ec!=1){
//    std::stringstream sstr; sstr<<"set register "<<index<<" channel("<<inedx<<") failed";
//    m_parent->recive_text(1,sstr.str()); return;}
//  std::stringstream sstr; sstr<<"set register "<<index<<" value: "<<value<<"("<<
//    dac<<"V)"; m_parent->recive_text(0,sstr.str());
//}
//void pwb::set_c0_adpator(){
//  QString text = ui.lineEdit_2->text();
//  float result = std::nanf("");
//  try{ result = std::stof(text.toStdString().c_str());
//  }catch(std::invalid_argument const& e){
//    m_parent->recive_text(1,"a float needed"); return;}
//  std::cout<<result<<std::endl;
//  if (result<0 || result>2.5){
//    m_parent->recive_text(1,"dac range [0,2.5](V)");
//    return; }
//  ui.horizontalSlider_5->setValue(65535*result/2.5);
//  info_out(ui.horizontalSlider_5->value());
//  emit aaa();
//}
void pwb::set_c0(){ }
void pwb::set_c0_adpator() {}
//---------------------------------------------------------------------
void pwb::stop(){
  m_timer.stop();
}

void pwb::update(){
  m_timer.start();
}


