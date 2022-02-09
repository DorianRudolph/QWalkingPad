// QWalkingPad - Simple desktop application for controlling the Kingsmith WalkingPad over BLE
// Copyright (C) 2021 Dorian Rudolph
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#include "MainWindow.h"
#include <QStyleFactory>
#include <QLoggingCategory>
#include <singleapplication.h>

int main(int argc, char *argv[]) {
  QCoreApplication::setOrganizationName("Dorian Rudolph");
  QCoreApplication::setOrganizationDomain("dorianrudolph.com");
  QCoreApplication::setApplicationName("QWalkingPad");

  SingleApplication app(argc, argv, true);

  if (app.isPrimary()) {
    if (argc > 1) return 1;
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
