#include <QApplication>
#include <thread>
#include <iostream>
#include "monitor.h"
#include <QtCharts/QChartView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QGraphicsLayout>
#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif
#include "mainwindow.h"
#include "monitors.h"
#include "switch.h"
int main(int argc, char *argv[]){
  QApplication app(argc, argv);

  mainwindow p;
  //window.setGeometry(0,0,770,830);
  monitors window(&p,nullptr);
  window.show();
  //window.show();
 
  //SwitchButton sw;
  //sw.show();
 
  return app.exec();
}
