#include "MainWindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

  QCoreApplication::setOrganizationName("Dorian Rudolph");
  QCoreApplication::setOrganizationDomain("dorianrudolph.com");
  QCoreApplication::setApplicationName("QWalkingPad");

  QApplication a(argc, argv);
  MainWindow win;
  win.show();
  return QApplication::exec();
}
