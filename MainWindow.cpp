#include "MainWindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QLoggingCategory>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QLabel>

MainWindow::MainWindow() {
  setupMenu();
  startDiscovering();
}

void MainWindow::setupMenu() {
  menuBar()->setNativeMenuBar(false);
  menuBar()->show();
  connectMenu = menuBar()->addMenu("&Connect");

  disconnectAction = new QAction("Disconnect", this);
  connect(disconnectAction, &QAction::triggered, this, &MainWindow::disconnect);
  disconnectAction->setDisabled(true);
  connectMenu->addAction(disconnectAction);

  scanAction = new QAction("Scan", this);
  connect(scanAction, &QAction::triggered, this, &MainWindow::scan);
  scanAction->setDisabled(true);
  connectMenu->addAction(scanAction);

  connectMenu->addSeparator();

  statusLabel = new QLabel("Disconnected");
  statusBar()->addPermanentWidget(statusLabel);
  statusBar()->showMessage("Scanning...", 0);
}

void MainWindow::startDiscovering() {
  deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
  deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(2000);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &MainWindow::scanFinished);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &MainWindow::addDevice);
  //TODO handle device updated
  deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void MainWindow::disconnect() {

}

void MainWindow::addDevice(const QBluetoothDeviceInfo &device) {
  if (!(device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
    return;
  qDebug() << "Found" << device.name() << device.address();
  auto deviceAction = new QAction(device.name() + " (" + device.address().toString() + ")", this);
  deviceAction->setData(devices.size());
  connect(disconnectAction, &QAction::triggered, this, &MainWindow::disconnect);

  devices.append(device);
  for (auto a : connectActions) {
    connectMenu->removeAction(a);
  }

  connectActions.append(deviceAction);
  std::sort(connectActions.begin(), connectActions.end(), [](const auto a, const auto b) {
    return a->text() < b->text(); });
  connectMenu->addActions(connectActions);
}

void MainWindow::scanFinished() {
  statusBar()->showMessage("Scan finished", 0);
  scanAction->setDisabled(false);
}

void MainWindow::scan() {
  for (auto a : connectActions) {
    connectMenu->removeAction(a);
    delete a;
  }
  devices.clear();
  connectActions.clear();
  scanAction->setDisabled(true);
  deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void MainWindow::connectDevice() {
  auto action = qobject_cast<QAction*>(sender());
  auto device = devices[action->data().toInt()];
  qDebug() << "Select" << device.name();
}

