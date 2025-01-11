#include "FrameLabel.h"

FrameLabel::FrameLabel(QWidget *parent) : QLabel(parent) {
  startPoint.setX(0);
  startPoint.setY(0);
  mouseCursorPos.setX(0);
  mouseCursorPos.setY(0);
  drawBox = false;
  mouseData.leftButtonRelease = false;
  mouseData.rightButtonRelease = false;
  createContextMenu();
}

FrameLabel::~FrameLabel() { delete menu; }

void FrameLabel::mouseMoveEvent(QMouseEvent *ev) {
  // Save mouse cursor position
  setMouseCursorPos(ev->pos());
  // Update box width and height if box drawing is in progress
  if (drawBox) {
    box->setWidth(getMouseCursorPos().x() - startPoint.x());
    box->setHeight(getMouseCursorPos().y() - startPoint.y());
  }
  // Inform main window of mouse move event
  emit onMouseMoveEvent();
}

void FrameLabel::setMouseCursorPos(QPoint input) { mouseCursorPos = input; }

QPoint FrameLabel::getMouseCursorPos() { return mouseCursorPos; }

void FrameLabel::mouseReleaseEvent(QMouseEvent *ev) {
  // Update cursor position
  setMouseCursorPos(ev->pos());
  // On left mouse button release
  if (ev->button() == Qt::LeftButton) {
    // Set leftButtonRelease flag to TRUE
    mouseData.leftButtonRelease = true;
    if (drawBox) {
      // Stop drawing box
      drawBox = false;
      // Save box dimensions
      mouseData.selectionBox.setX(box->left());
      mouseData.selectionBox.setY(box->top());
      mouseData.selectionBox.setWidth(box->width());
      mouseData.selectionBox.setHeight(box->height());
      // Set leftButtonRelease flag to TRUE
      mouseData.leftButtonRelease = true;
      // Inform main window of event
      emit newMouseData(mouseData);
    }
    // Set leftButtonRelease flag to FALSE
    mouseData.leftButtonRelease = false;
  }
  // On right mouse button release
  else if (ev->button() == Qt::RightButton) {
    // If user presses (and then releases) the right mouse button while drawing
    // box, stop drawing box
    if (drawBox)
      drawBox = false;
    else {
      // Show context menu
      menu->exec(ev->globalPosition().toPoint());
    }
  }
}

void FrameLabel::mousePressEvent(QMouseEvent *ev) {
  // Update cursor position
  setMouseCursorPos(ev->pos());
  ;
  if (ev->button() == Qt::LeftButton) {
    // Start drawing box
    startPoint = ev->pos();
    box = new QRect(startPoint.x(), startPoint.y(), 0, 0);
    drawBox = true;
  }
}

void FrameLabel::paintEvent(QPaintEvent *ev) {
  QLabel::paintEvent(ev);
  QPainter painter(this);
  // Draw box
  if (drawBox) {
    painter.setPen(Qt::blue);
    painter.drawRect(*box);
  }
}

void FrameLabel::createContextMenu() {
  // Create top-level menu object
  menu = new QMenu(this);
  // Add actions
  QAction *action;

  action = new QAction(this);
  action->setText(tr("Reset ROI"));
  menu->addAction(action);

  action = new QAction(this);
  action->setText(tr("Scale to Fit Frame"));
  action->setCheckable(true);
  menu->addAction(action);

  action = new QAction(this);
  action->setText(tr("Show Original Frame"));
  action->setCheckable(true);
  menu->addAction(action);

  menu->addSeparator();
}
