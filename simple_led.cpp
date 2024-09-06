#include <cmath>
#include <type_traits>

#include "QPainter"
#include "simple_led.h"
/* k_custom k_red k_green k_blue k_yellow k_orange */
simple_led::color_group_t simple_led::sm_color_palette[6] {
  {QColor(0,0,0),QColor(0,0,0),QColor(0,0,0),QColor(0,0,0)}
  ,{QColor(255,0,0),QColor(191,0,0),QColor(28,0,0),QColor(128,0,0)}
  ,{QColor(0,255,0),QColor(0,191,0),QColor(0,28,0),QColor(0,128,0)}
  ,{QColor(0,0,255),QColor(0,0,191),QColor(0,0,28),QColor(0,0,128)}
  ,{QColor(255,255,0),QColor(191,191,0),QColor(28,28,0),QColor(128,128,0)}
  ,{QColor(255,165,0),QColor(255,113,1),QColor(20,8,5),QColor(99,39,24)} };

simple_led::simple_led(QWidget* parent, self_t::e_color color):
  QAbstractButton(parent) ,m_color(color){ 
    setCheckable(true);/*setMinimumSize(30,30);*/
    this->setGeometry(5,5,56,56);
}

void simple_led::status(simple_led::e_status st){
  switch(st){
    case e_status::k_on:
      reset_status(); setChecked(true); m_status = e_status::k_on; break;
    case e_status::k_off:
      reset_status(); break;
    case e_status::k_blink:
      reset_status();
      if (!m_blink_timer){
        m_blink_timer = new QTimer(this);
        connect(m_blink_timer,&QTimer::timeout,this,&simple_led::blink_timer_timeout); }
      m_blink_timer->setInterval(m_interval);
      m_blink_timer->start();
      m_status = e_status::k_blink;
      break;
  }
  this->update(); }

void simple_led::paintEvent(QPaintEvent* event){
  Q_UNUSED(event);
  auto minsize = std::min(width(),height());
  auto radial_gradient = QRadialGradient(QPointF(-500,-500),1500,QPointF(-500,-500));
  radial_gradient.setColorAt(0,QColor(224,224,224));
  radial_gradient.setColorAt(1,QColor(28,28,28));

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.translate(width()/2,height()/2);
  painter.scale(minsize/1000.,minsize/1000.);
  painter.setBrush(QBrush(radial_gradient));
  painter.drawEllipse(QPointF(0,0),500,500);

  radial_gradient = QRadialGradient(QPointF(500,500),1500,QPointF(500,500));
  radial_gradient.setColorAt(0,QColor(224,224,224));
  radial_gradient.setColorAt(1,QColor(28,28,28));

  painter.setBrush(QBrush(radial_gradient));
  painter.drawEllipse(QPointF(0,0),450,450);

  if(this->isChecked()){
    radial_gradient = QRadialGradient(QPointF(-500,-500),1500,QPointF(-500,-500));
    radial_gradient.setColorAt(0,sm_color_palette[(std::underlying_type<e_color>::type)m_color].m_on0);
    radial_gradient.setColorAt(1,sm_color_palette[(std::underlying_type<e_color>::type)m_color].m_on1); }
  else{
    radial_gradient = QRadialGradient(QPointF(500,500),1500,QPointF(500,500));
    radial_gradient.setColorAt(0,sm_color_palette[(std::underlying_type<e_color>::type)m_color].m_off0);
    radial_gradient.setColorAt(1,sm_color_palette[(std::underlying_type<e_color>::type)m_color].m_off1); }
  painter.setBrush(QBrush(radial_gradient));
  painter.drawEllipse(QPoint(0,0),400,400); }

void simple_led::resizeEvent(QResizeEvent*){ this->update(); }
void simple_led::mousePressEvent(QMouseEvent*){ }
void simple_led::blink_timer_timeout(){ this->setChecked(!this->isChecked()); }
void simple_led::reset_status(){
  if (m_blink_timer && m_blink_timer->isActive()) m_blink_timer->stop();
  setChecked(false); m_status = e_status::k_off; }
