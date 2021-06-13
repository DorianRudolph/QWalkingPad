#include "MainWindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
  QApplication::setStyle(QStyleFactory::create("Fusion"));
  QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
  QApplication a(argc, argv);
  MainWindow win;
  win.show();
  return QApplication::exec();
}
