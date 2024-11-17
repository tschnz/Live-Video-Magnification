#include "ui/MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  // Show main window
  QApplication a(argc, argv);
  MainWindow w;

  w.show();
  // Start event loop
  return a.exec();
}
