#include <QObject>
#include "qcustomplot.h"

class axis_tag : public QObject{
  Q_OBJECT

public:
  explicit axis_tag(QCPAxis*);
  virtual ~axis_tag();

  void setPen(QPen const&);
  void setBrush(const QBrush&);
  void setText(const QString&);

  QPen pen() const {return mLabel->pen();}
  QBrush brush() const {return mLabel->brush();}
  QString text() const {return mLabel->text();}

  void updatePosition(double value);

protected:
  QCPAxis* mAxis;
  QPointer<QCPItemTracer> mDummyTracer;
  QPointer<QCPItemLine> mArrow;
  QPointer<QCPItemText> mLabel;

};
