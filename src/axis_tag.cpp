#include "axis_tag.h"

axis_tag::axis_tag(QCPAxis* p):
  QObject(p)
  ,mAxis(p){
  mDummyTracer = new QCPItemTracer(mAxis->parentPlot());
  mDummyTracer->setVisible(true);
  mDummyTracer->position->setTypeX(QCPItemPosition::ptAxisRectRatio);
  mDummyTracer->position->setTypeY(QCPItemPosition::ptPlotCoords);
  mDummyTracer->position->setAxisRect(mAxis->axisRect());
  mDummyTracer->position->setAxes(0,mAxis);
  mDummyTracer->position->setCoords(1,0);

  mArrow = new QCPItemLine(mAxis->parentPlot());
  mArrow->setLayer("overlay");
  mArrow->setClipToAxisRect(false);
  mArrow->setHead(QCPLineEnding::esSpikeArrow);
  mArrow->end->setParentAnchor(mDummyTracer->position);
  mArrow->start->setParentAnchor(mArrow->end);
  mArrow->start->setCoords(15,0);

  mLabel = new QCPItemText(mAxis->parentPlot());
  mLabel->setLayer("overlay");
  mLabel->setClipToAxisRect(false);
  mLabel->setPadding(QMargins(3,0,3,0));
  mLabel->setBrush(QBrush(Qt::white));
  mLabel->setPen(QPen(Qt::blue));
  mLabel->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  mLabel->position->setParentAnchor(mArrow->start);
}
axis_tag::~axis_tag(){
  if (mDummyTracer) mDummyTracer->parentPlot()->removeItem(mDummyTracer);
  if (mArrow) mArrow->parentPlot()->removeItem(mArrow);
  if (mLabel) mLabel->parentPlot()->removeItem(mLabel); }
void axis_tag::setPen(QPen const& p){ mArrow->setPen(p); mLabel->setPen(p); }
void axis_tag::setBrush(QBrush const& b){ mLabel->setBrush(b); }
void axis_tag::setText(QString const& text){ mLabel->setText(text); }
void axis_tag::updatePosition(double v){
  mDummyTracer->position->setCoords(1,v); mArrow->end->setCoords(mAxis->offset(),0); }
