// -*-c++-*-

/*!
	\file field_canvas.h
	\brief field canvas class Header File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.	If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef SOCCERWINDOW2_QT_FIELD_CANVAS_H
#define SOCCERWINDOW2_QT_FIELD_CANVAS_H

//#define USE_GLWIDGET

#ifdef USE_GLWIDGET
#include <QGLWidget>
#else
#include <QWidget>
#endif

#include "mouse_state.h"

#include <boost/shared_ptr.hpp>

#include <vector>

class QContextMenuEvent;
class QMenu;
class QMouseEvent;
class QPaintEvent;
class QPixmap;
class QPoint;
class QPointF;
class QTimer;

class MainData;
class PainterInterface;
class FieldPainter;

//! main soccer field canvas class
class FieldCanvas
    :
#ifdef USE_GLWIDGET
    public QGLWidget
#else
    public QWidget
#endif
{
    Q_OBJECT

private:

    MainData & M_main_data;

    bool M_redraw_all;

    // for more explanation on QMenu : http://doc.qt.io/archives/qt-4.8/qmenu.html

    QMenu * M_normal_menu;
    QMenu * M_system_menu;
    QMenu * M_monitor_menu;

    //! 0: left, 1: middle, 2: right
    MouseState M_mouse_state[3];
    QPoint M_measure_first_length_point;

    QTimer * M_cursor_timer;

    int M_draw_type;
    boost::shared_ptr< FieldPainter > M_field_painter;

    std::vector< boost::shared_ptr< PainterInterface > > M_painters;

    QPoint M_player_dragged_point;

    // not used
    FieldCanvas( const FieldCanvas & );
    const FieldCanvas & operator=( const FieldCanvas & );

public:

    explicit
    FieldCanvas( MainData & main_data );

    ~FieldCanvas();

    void createPainters();

    void setNormalMenu( QMenu * menu );
    void setSystemMenu( QMenu * menu );
    void setMonitorMenu( QMenu * menu );

    void draw( QPainter & painter );

private:

    void drawMouseMeasure( QPainter & painter );
    void createBallMovePath( const QPoint & start_point,
                             const QPoint & end_point,
                             QPainterPath & mark_path,
                             QPainterPath & text_path );
    void createPlayerMovePath( const QPoint & start_point,
                               const QPoint & end_point,
                               QPainterPath & mark_path,
                               QPainterPath & text_path );

    void drawDraggedPlayer( QPainter & painter );

    void grabPlayer( const QPoint & point );
    void dragPlayer( const QPoint & point );

protected:

    void mouseDoubleClickEvent( QMouseEvent * event );
    void mousePressEvent( QMouseEvent * event );
    void mouseReleaseEvent( QMouseEvent * event );
    void mouseMoveEvent( QMouseEvent * event );

    void paintEvent( QPaintEvent * );

public slots:

    void setRedrawAllFlag();
    void redrawAll();

    void dropBall();
    void freeKickLeft();
    void freeKickRight();

    void changePlayMode( int mode );

private slots:

    void hideCursor();

signals:

    void dropBall( const QPoint & pos );
    void freeKickLeft( const QPoint & pos );
    void freeKickRight( const QPoint & pos );

    void playModeChanged( int mode,
                          const QPoint & pos );

    void playerMoved( const QPoint & point );

    void focusChanged( const QPoint & point );
    void mouseMoved( const QPoint & point );

};

#endif
