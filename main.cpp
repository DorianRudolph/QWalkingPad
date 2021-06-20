#include "MainWindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QLoggingCategory>

#include <QFile>

void test(){
  qDebug() << QString(" 123 ").toInt();
  {
    QFile f("test.txt");
    f.open(QFile::WriteOnly | QFile::Text);

    while (!f.atEnd()) {
      auto line = f.readLine();
      qDebug() << "Line" << line;
    }
    //f.write("asdf\n");
    //f.resize(0);
    //f.write("oipu\n");
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

  QCoreApplication::setOrganizationName("Dorian Rudolph");
  QCoreApplication::setOrganizationDomain("dorianrudolph.com");
  QCoreApplication::setApplicationName("QWalkingPad");

  if (!Settings().getUseSystemTheme()) {
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QIcon::setThemeName("Fusion");
  }

  QApplication a(argc, argv);
  MainWindow win;
  win.show();
  return QApplication::exec();
}
