/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->FrameLabel.h                                       */
/*                                                                                  */
/* This program is free software: you can redistribute it and/or modify             */
/* it under the terms of the GNU General Public License as published by             */
/* the Free Software Foundation, either version 3 of the License, or                */
/* (at your option) any later version.                                              */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful,                  */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    */
/* GNU General Public License for more details.                                     */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>.            */
/************************************************************************************/

#ifndef FRAMELABEL_H
#define FRAMELABEL_H

// Qt
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QLabel>
#include <QMenu>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
// Local
#include "main/other/Structures.h"

class FrameLabel : public QLabel
{
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

#endif // FRAMELABEL_H
