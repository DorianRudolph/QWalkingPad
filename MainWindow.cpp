#include "MainWindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QLoggingCategory>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>

MainWindow::MainWindow() {
  setupMenu();
  setupTimer();
  startDiscovering();
}

void MainWindow::setupMenu() {
  menuBar()->setNativeMenuBar(false);
  menuBar()->show();
  connectMenu = menuBar()->addMenu("&Connect");

  disconnectAction = new QAction("&Disconnect", this);
  connect(disconnectAction, &QAction::triggered, this, &MainWindow::disconnect);
  disconnectAction->setDisabled(true);
  connectMenu->addAction(disconnectAction);

  scanAction = new QAction("&Scan", this);
  connect(scanAction, &QAction::triggered, this, &MainWindow::scan);
  scanAction->setDisabled(true);
  connectMenu->addAction(scanAction);

  connectMenu->addSeparator();

  statusLabel = new QLabel("Disconnected");
  statusBar()->addPermanentWidget(statusLabel);
  messageLabel = new QLabel("Scanning...");
  statusBar()->addWidget(messageLabel);
}

void MainWindow::setupTimer() {
  timer = new QTimer(this);
  timer->setInterval(1000);
  connect(timer, &QTimer::timeout, this, &MainWindow::tick);
  timer->start();
}

void MainWindow::startDiscovering() {
  deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
  deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(2000);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &MainWindow::scanFinished);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &MainWindow::scanFinished);
  connect(deviceDiscoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &MainWindow::scanError);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &MainWindow::addDevice);
  //TODO handle device updated
  deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void MainWindow::disconnect() {
  state = DISCONNECTING;
  bleController->disconnectFromDevice();
  showMessage("Disconnecting...");
  if (bleController->state() != QLowEnergyController::ClosingState){
    disconnected();
  }
}

void MainWindow::addDevice(const QBluetoothDeviceInfo &device) {
  if (!(device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
    return;
  qDebug() << "Found" << device.name() << device.address();
  auto deviceAction = new QAction(device.name() + " (" + device.address().toString() + ")", this);
  deviceAction->setData(devices.size());
  connect(deviceAction, &QAction::triggered, this, &MainWindow::connectDevice);

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
  if (state == DISCONNECTED){
    showMessage("Scan finished");
  }
  scanAction->setDisabled(false);
}

void MainWindow::scanError(QBluetoothDeviceDiscoveryAgent::Error error) {
  auto err = deviceDiscoveryAgent->errorString();
  qCritical() << "Error while scanning for devices" << err;
  showMessage("Scan error: " + err);
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
  currentDevice = &devices[action->data().toInt()];
  state = CONNECTING;
  setConnectActionEnabled(false);
  disconnectAction->setEnabled(true);
  showMessage("Connecting to " + currentDevice->name() + " ...");
  if (!bleController) {
    delete bleController;
  }
  service = nullptr;
  bleController = QLowEnergyController::createCentral(*currentDevice, this);
  connect(bleController, &QLowEnergyController::serviceDiscovered, this, &MainWindow::serviceDiscovered);
  connect(bleController, &QLowEnergyController::discoveryFinished, this, &MainWindow::discoveryFinished);
  connect(bleController, &QLowEnergyController::connected, this, &MainWindow::connected);
  connect(bleController, &QLowEnergyController::disconnected, this, &MainWindow::disconnected);
  connect(bleController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &MainWindow::connectError);
  bleController->connectToDevice();
}

void MainWindow::connected() {
  qDebug("Connected");
  bleController->discoverServices();
  setStatus("Connected (" + currentDevice->name() + ")");
  showMessage("");
}

void MainWindow::disconnected() {
  qDebug("Disconnected");
  setConnectActionEnabled(true);
  setStatus("Disconnected");
  bleController->disconnectFromDevice();
  state = DISCONNECTED;
  disconnectAction->setEnabled(false);
  showMessage("");
}

void MainWindow::serviceDiscovered(const QBluetoothUuid &gatt) {
  qDebug() << "Service discovered" << gatt;
  if (gatt.toUInt16() == 0xfe00) {
    service = bleController->createServiceObject(gatt, bleController);
    Q_ASSERT(service->state() == QLowEnergyService::DiscoveryRequired);
    connect(service, &QLowEnergyService::stateChanged,this, &MainWindow::serviceStateChanged);
    service->discoverDetails();
  }
}

void MainWindow::serviceStateChanged(QLowEnergyService::ServiceState newState) {
  if (newState != QLowEnergyService::ServiceDiscovered) return;
  connect(service, &QLowEnergyService::characteristicChanged, this, &MainWindow::characteristicChanged);
  auto readChar = service->characteristic(QBluetoothUuid((quint16)0xfe01));
  writeChar = service->characteristic(QBluetoothUuid((quint16)0xfe01));
  if (!readChar.isValid() || !writeChar.isValid()) {
    qCritical("Characteristics not found");
    handleInvalidService();
  }
  Q_ASSERT(readChar.isValid());
  auto notifyConfig = readChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
  service->writeDescriptor(notifyConfig, QByteArray::fromHex("0100"));
  state = CONNECTED;
}

void MainWindow::characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value){
  qDebug() << "Characteristic Changed" << c.uuid() << value.toHex();
}

void MainWindow::discoveryFinished() {
  qDebug() << "Discovery Finished";
  if (!service) {
    handleInvalidService();
  }
}

void MainWindow::connectError(QLowEnergyController::Error error) {
  auto err {bleController->errorString()};
  qCritical() << "Error while connecting to " << currentDevice->name() << err;
  showMessage("Connection error: " + err);
  QMessageBox::warning(this, "Connection Error", "Error while connecting: " + err + " ");
  disconnected();
}

void MainWindow::showMessage(const QString &message) {
  messageLabel->setText(message);
}

void MainWindow::setStatus(const QString &status) {
  statusLabel->setText(status);
}

void MainWindow::tick() {
  if (state != CONNECTED) return;
  qDebug("tick");
  auto writeChar = service->characteristic(QBluetoothUuid((quint16)0xfe02));
  Q_ASSERT(writeChar.isValid());
  service->writeCharacteristic(writeChar, QByteArray::fromHex("f7a20000a2fd"));
}

void MainWindow::setConnectActionEnabled(bool enabled) {
  for (auto a : connectActions) {
    a->setEnabled(enabled);
  }
}

void MainWindow::handleInvalidService() {
  qCritical("No WalkingPad Service found");
  QMessageBox::warning(this, "No WalkingPad Service", currentDevice->name() + " does not appear to be a WalkingPad. Disconnecting...");
  showMessage("No WalkingPad Service. Disconnecting...");
  bleController->disconnectFromDevice();
}
