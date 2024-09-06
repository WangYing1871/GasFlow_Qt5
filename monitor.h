#ifndef monitor_H
#define monitor_H 1 
#include <iostream>
#include <vector>
#include <deque>
#include <array>

#include <QtCharts/QChart>
#include <QtCore/QTimer>
#include <QtCharts/QLineSeries>
#include <QGraphicsSceneMouseEvent>
#include <QtCharts/QChartView>

#include "util.h"

#ifdef QT_CHARTS_BEGIN_NAMESPACE
QT_CHARTS_BEGIN_NAMESPACE
#endif
class QSplineSeries;
class QLineSeries;
class QValueAxis;
#ifdef QT_CHARTS_END_NAMESPACE
QT_CHARTS_END_NAMESPACE
#endif

#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif


class monitor : public QChart{
  Q_OBJECT
public:

  template <class _tp,size_t N>
  struct graph{
    _tp* m_series;
    QValueAxis* m_axis[N];
    QChart* m_chart;


    void set_axis_title(std::string const& a, std::string const& b, std::string const& c){
    }
    void link() {for(auto&& x : m_axis)  m_series->addAxis(x);}
  };

  

public:
  monitor(QGraphicsItem* parent = 0, Qt::WindowFlags wFlags=Qt::Widget);
  virtual ~monitor();

  void format();
  void format_line();
  void start(uint32_t = 1000);

  template <class _tp, class _up>
  inline monitor* setxy(_tp const& x, _up const& y) {m_x = x, m_y = y; return this;}

  QValueAxis* AxisX() const {return m_axisX;}
  QValueAxis* AxisY() const {return m_axisY;}

  std::array<qreal,2> getdxy() const;

  double adjust_y() const;
  inline void enable_adjust_y() {_is_adjust_y = true;}
  inline void clear() {
    m_lineseries->clear();
    m_cache.clear();
    m_cache.push_back({-2,0});
    m_cache.push_back({-1,0});
    m_cache.push_back({0,0});
    m_x = m_y = 0.;
  }
  void stop();
  uint32_t m_cache_size = 128;




public slots:
  void handleTimeout();
  void handleTimeout_line();

private:
  QTimer m_timer;
  QSplineSeries* m_series,*m_series0;
  QLineSeries* m_lineseries;
  std::string m_title;
  QValueAxis* m_axisX,* m_axisY;
  qreal m_step, m_x, m_y;
  bool _is_button = false;
  std::deque<std::array<qreal,2>> m_cache;
  std::string m_file;
  bool _is_adjust_y = false;


protected:
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public:
#ifdef D_develop
  void axes_db() const;

#endif


};
#endif
