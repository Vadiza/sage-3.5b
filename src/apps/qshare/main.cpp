
#include "capturewindow.h"


int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  CaptureWindow *widget = new CaptureWindow;

#if defined(__APPLE__)
  //widget->setWindowIcon();
#endif

  widget->show();

  return app.exec();
}
