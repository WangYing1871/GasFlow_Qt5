#include <iostream>
#include <cmath>
#include <sstream>
#include <fstream>
#include <ranges>
#include <QFont>
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCore/QRandomGenerator>
#include <QtCore/QDebug>

#include "monitor.h"
#include "form.h"

monitor::monitor(
    QGraphicsItem* parent
    ,Qt::WindowFlags wFlags):
  QChart(QChart::ChartTypeCartesian,parent,wFlags)
  ,m_series(0)
  ,m_lineseries(0)
  ,m_axisX(new QValueAxis())
  ,m_axisY(new QValueAxis())
  ,m_step(0) {


  AxisX()->setGridLineVisible(false);
  AxisY()->setGridLineVisible(false);

    
  m_cache.push_back({-2,0});
  m_cache.push_back({-1,0});
  m_cache.push_back({0,0});
  addAxis(m_axisX,Qt::AlignBottom); addAxis(m_axisY,Qt::AlignLeft);
}


void monitor::format(){
  m_series = new QSplineSeries(this);
  QPen green(Qt::blue);
  green.setWidth(1);
  m_series->setPen(green);
  addSeries(m_series);
  addAxis(m_axisX,Qt::AlignBottom); addAxis(m_axisY,Qt::AlignLeft);
  m_series->attachAxis(m_axisX);
  m_series->attachAxis(m_axisY);
  m_axisX->setTickCount(5);
  m_axisX->setRange(0,128);

  QObject::connect(&m_timer,&QTimer::timeout,this,&monitor::handleTimeout);
}
void monitor::format_line(){
  m_lineseries = new QLineSeries(this);
  //m_lineseries->setMarkershape(QScatterSeries::MarkerShapeRectangle);
  //m_lineseries->setMarkerSize(1);
  QPen green(Qt::blue);
  green.setWidth(1);
  m_lineseries->setPen(green);
  addSeries(m_lineseries);
  m_lineseries->attachAxis(m_axisX);
  m_lineseries->attachAxis(m_axisY);
  m_axisX->setTickCount(10);
  m_axisX->setRange(0,10);
  m_axisY->setRange(-5,50);

  QObject::connect(&m_timer,&QTimer::timeout,this,&monitor::handleTimeout_line);
}

void monitor::start(uint32_t inv){
  m_timer.setInterval(inv);
  m_timer.start();
}

monitor::~monitor() {}

void monitor::handleTimeout(){
  if (m_cache.size()>=1024) { m_cache.clear(); m_series->clear(); }
  m_series->append(m_x,m_y);
  m_cache.emplace_back(std::array<qreal,2>{m_x,m_y});
  qreal x = plotArea().width() / m_axisX->tickCount();
  
  //scroll(x/10,0);
  scroll(0,0);
}

void monitor::handleTimeout_line(){

  if (m_cache.size()>=m_cache_size){
    for(int i=0; i<m_cache_size/2; ++i) m_cache.pop_front();
  }
  
  m_lineseries->append(m_x,m_y);
  m_cache.emplace_back(std::array<qreal,2>{m_x,m_y});

  m_axisX->setRange(m_cache.front()[0],m_x+0.1*m_cache_size);
  
  if (_is_adjust_y){ double y = adjust_y(); m_axisY->setRange(0.99*y,1.01*y); }
  //qreal x = plotArea().width() / m_axisX->tickCount();

}

void monitor::mousePressEvent(QGraphicsSceneMouseEvent *event){
  std::cerr<<"wangying0 "<<event->lastPos().x()<<" "<<event->lastPos().y()
    <<" "<<event->pos().x()<<" "<<event->pos().y()
    <<" |"<<m_axisX->min()<<"| "<<m_axisY->max()<<std::endl;
  _is_button = true;
  //axes_db();


  

  
}
void monitor::mouseMoveEvent(QGraphicsSceneMouseEvent* event){
  if (_is_button==true){ std::cerr<<"ground "; return; }
  std::cerr<<"fly"<<std::endl;
}
void monitor::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
  std::cerr<<"wangying1 "<<event->lastPos().x()<<" "<<event->lastPos().y()
    <<" "<<event->pos().x()<<" "<<event->pos().y()<<std::endl;
  _is_button = false;
}

std::array<qreal,2> monitor::getdxy() const{
  if (m_cache.size()<=3) return {0,0};
  auto p0_iter = std::prev(m_cache.end());
  auto p1_iter = std::prev(p0_iter); auto p2_iter = std::prev(p1_iter);
  auto p0 = *p0_iter; auto p1 = *p1_iter; auto p2 = *p2_iter;
  return { p0[0]+p2[0]-2*p1[0] ,p0[1]+p2[1]-2*p1[1] };
}

double monitor::adjust_y() const{
  auto begin = std::prev(std::end(m_cache),3);
  double rt = 0.;
  for (auto iter = begin; iter != std::end(m_cache); ++iter) rt += iter->at(1);
  return rt/3.; };

void monitor::stop(){ m_timer.stop(); }


#ifdef D_develop
void monitor::axes_db() const{
  //auto xya = this->axes();
  //debug_out(xya.count());
  QFont xf("Arial",9,2,true);
  m_axisX->setTitleFont(xf);
  m_axisX->setTitleText("XXX");
}
#endif
