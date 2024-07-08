#ifndef run_H
#define run_H 1 
#include <QWidget>
#include <vector>

#include "ui_run.h"

class mainwindow;
class run : public QWidget{
  Q_OBJECT
public: 
  run(QWidget* parent=0);
  ~run() noexcept = default;
  mainwindow* parent() const {return m_parent;}
  void parent(mainwindow* value) {m_parent = value;}

  std::vector<QWidget*> m_childs;

protected:
  void closeEvent(QCloseEvent* event) override;

public slots:
  void recycle_mode();
  void dump_mode();
  void self_cycle_mode();

private:
  mainwindow* m_parent;
  Ui::Run ui;
};
#endif
