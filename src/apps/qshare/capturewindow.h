#ifndef CAPTUREWINDOW_H
#define CAPTUREWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QtDebug>
#include <QTimer>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QDesktopWidget>
#include "ui_capturewindow.h"

#if defined(__APPLE__)
// Mac Headers
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif


class CaptureWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit CaptureWindow(QWidget *parent = 0);

protected:
  void closeEvent(QCloseEvent *event);

public slots:
  void onStart();
  void onStop();
  void onPause();
  void update();
  void fsIP(QString);
  void frameRate(int);
  void compression(int);
  void cursor(int);

private:
  Ui_CaptureWindow ui;
  int fps;
  double startt;
  int count;
  bool started;
  bool dxt;
  bool showcursor;
  QTimer *timer;
  QImage *cursor_icon, *desktop;
  int WW, HH;
  QString fsip;

#if defined(__APPLE__)
  CGLContextObj  glContextObj;
#endif

private:
  void capture(char* m_pFrameRGB,int x,int y,int cx,int cy);
  void drawCursor(char *pixels, int nbytes, int flip = 1);
  bool screenHasCursor();
};

#endif // CAPTUREWINDOW_H
