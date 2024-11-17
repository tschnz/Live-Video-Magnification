#pragma once

// Qt
#include <QLabel>
#include <QMenu>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
// Local
#include "../other/Structures.h"

class FrameLabel : public QLabel {
  Q_OBJECT

public:
  FrameLabel(QWidget *parent = 0);
  ~FrameLabel();
  void setMouseCursorPos(QPoint);
  QPoint getMouseCursorPos();
  QMenu *menu;

private:
  void createContextMenu();
  MouseData mouseData;
  QPoint startPoint;
  QPoint mouseCursorPos;
  bool drawBox;
  QRect *box;

protected:
  void mouseMoveEvent(QMouseEvent *ev);
  void mousePressEvent(QMouseEvent *ev);
  void mouseReleaseEvent(QMouseEvent *ev);
  void paintEvent(QPaintEvent *ev);

signals:
  void newMouseData(struct MouseData mouseData);
  void onMouseMoveEvent();
};