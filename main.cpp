#include "MainWindow.h"
#include <QStyleFactory>
#include <QLoggingCategory>
#include <singleapplication.h>

int main(int argc, char *argv[]) {
  //QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

  QCoreApplication::setOrganizationName("Dorian Rudolph");
  QCoreApplication::setOrganizationDomain("dorianrudolph.com");
  QCoreApplication::setApplicationName("QWalkingPad");

  SingleApplication app(argc, argv, true);

  if (app.isPrimary()) {
    if (argc > 1) return 1;
    qDebug() << QIcon::themeName();
    if (!Settings().getUseSystemTheme())
      QApplication::setStyle(QStyleFactory::create("fusion"));
    MainWindow win;
    QObject::connect(&app, &SingleApplication::receivedMessage, &win, &MainWindow::receivedMessage);
    win.show();
    return QApplication::exec();
  } else {
    auto args = SingleApplication::arguments();
    args.pop_front();
    if (args.empty()) return 1;
    app.sendMessage(args.join(' ').toUtf8());
  }

}
