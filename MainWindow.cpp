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
#include <QMenuBar>
#include <QStatusBar>
#include <QLoggingCategory>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include "Protocol.h"
#include <QDateTime>
#include <QSlider>
#include "AbsoluteSliderStyle.h"
#include <QPushButton>
#include <QFontDatabase>
#include <QActionGroup>
#include <QHotkey>

MainWindow::MainWindow() {
  setWindowTitle("QWalkingPad");
  setupLayout();
  setupMenu();
  setupTimer();
  setupHotkeys();
  startDiscovering();
}

void MainWindow::showEvent(QShowEvent *event) {
  auto path = settings.getDataPath();
  if(!path.isEmpty()) {
    stats.load(path);
    if (!stats.hasFile()) {
      QMessageBox::warning(this, "Failed to load statistics", "Failed to parse file " + settings.getDataPath());
    }
    updateStatsLabel();
  }
}

QSlider * MainWindow::makeSpeedSlider() {
  auto slider = new QSlider(Qt::Orientation::Horizontal);
  slider->setFixedHeight(25);
  slider->setStyle(new AbsoluteSliderStyle(slider->style()));
  slider->setRange(0, maxSpeed); // TODO handle different maximum speeds
  slider->setTickInterval(10);
  slider->setTickPosition(QSlider::TicksBelow);
  slider->setTracking(false);
  slider->setPageStep(5);
  slider->setSingleStep(1);
  return slider;
}

void MainWindow::setupLayout() {
  centerWidget = new QWidget;
  centerWidget->setEnabled(false);

  auto vBox = new QVBoxLayout;
  vBox->setAlignment(Qt::AlignTop);
  centerWidget->setLayout(vBox);
  setCentralWidget(centerWidget);
  auto modeGroup = new QGroupBox("Mode");
  vBox->addWidget(modeGroup);
  auto hBox = new QHBoxLayout;
  modeGroup->setLayout(hBox);
  modeGroup->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  auto addModeButton = [&](const char *label, uint8_t mode, bool checked=false) {
    auto button = new QRadioButton(label);
    modeButtons[mode] = button;
    hBox->addWidget(button);
    button->setChecked(checked);
    connect(button, &QRadioButton::clicked, this, [this, mode]{
      setModeTime = QDateTime::currentMSecsSinceEpoch();
      send(Pad::setMode(mode));
    });
  };
  addModeButton("Sleep", Pad::MODE_SLEEP, true);
  addModeButton("Manual", Pad::MODE_MANUAL);
  addModeButton("Auto", Pad::MODE_AUTO);

  auto speedGroup = new QGroupBox("Speed");
  speedGroup->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  auto grid = new QGridLayout;
  speedGroup->setLayout(grid);
  vBox->addWidget(speedGroup);

  speedLabel = new QLabel;
  grid->addWidget(new QLabel("Current:"), 0, 0, Qt::AlignTop);
  grid->addWidget(speedLabel, 0, 1, Qt::AlignTop);

  speedSlider = makeSpeedSlider();
  connect(speedSlider, &QSlider::valueChanged, this, [=](auto speed){
    setSpeedTime = QDateTime::currentMSecsSinceEpoch();
    setSpeedWidgets(speed);
    send(Pad::setSpeed(speed));
  });
  connect(speedSlider, &QSlider::sliderMoved, this, &MainWindow::setSpeedWidgets);
  grid->addWidget(speedSlider, 0, 2);
  setSpeedWidgets(0);

  startSpeedLabel = new QLabel;
  grid->addWidget(new QLabel("Start:"), 1, 0, Qt::AlignTop);
  grid->addWidget(startSpeedLabel, 1, 1, Qt::AlignTop);

  startSpeedSlider = makeSpeedSlider();
  grid->addWidget(startSpeedSlider, 1, 2);
  connect(startSpeedSlider, &QSlider::valueChanged, this, [=](auto speed){
    setStartSpeedWidgets(speed);
    send(Pad::setStartSpeed(speed));
    if (settings.getUnifiedSpeed()){
      send(Pad::setSpeed(speed));
    }
  });
  connect(startSpeedSlider, &QSlider::sliderMoved, this, &MainWindow::setStartSpeedWidgets);
  setStartSpeedWidgets(20);

  startButton = new QPushButton("Start");
  connect(startButton, &QPushButton::pressed, this, [=](){ sendStart(); });
  grid->addWidget(startButton, 2, 0, 1, 2);

  auto statsGroup = new QGroupBox("Statistics");
  statsGroup->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
  vBox->addWidget(statsGroup);
  auto statsLayout = new QGridLayout;
  //statsLayout->setAlignment(Qt::AlignCenter);
  statsGroup->setLayout(statsLayout);
  QFont font("monospace", 20);
  font.setFixedPitch(true);
  statsLabel = new QLabel;
  statsLabel->setFont(font);
  statsLayout->addWidget(statsLabel, 0, 0, Qt::AlignCenter);
  updateStatsLabel();
}


void MainWindow::setSpeedWidgets(int speed) {
  speedSlider->setSliderPosition(speed);
  speedLabel->setText(QString("%1 km/h").arg(speed/10., 0, 'f', 1));
}

void MainWindow::setStartSpeedWidgets(int speed) {
  speed = std::max(speed, 5);
  startSpeedSlider->setSliderPosition(speed);
  startSpeedLabel->setText(QString("%1 km/h").arg(speed/10., 0, 'f', 1));
}

void MainWindow::setupMenu() {
  menuBar()->setNativeMenuBar(false);
  menuBar()->show();
  connectMenu = menuBar()->addMenu("&Connect");

  disconnectAction = connectMenu->addAction("&Disconnect");
  connect(disconnectAction, &QAction::triggered, this, &MainWindow::disconnect);
  disconnectAction->setDisabled(true);

  scanAction = connectMenu->addAction("&Scan");
  connect(scanAction, &QAction::triggered, this, &MainWindow::scan);
  scanAction->setDisabled(true);

  auto autoReconnectAction = connectMenu->addAction("&Reconnect Automatically");
  autoReconnectAction->setCheckable(true);
  autoReconnectAction->setChecked(settings.getAutoReconnect());
  connect(autoReconnectAction, &QAction::triggered, [this](auto checked) {
    settings.setAutoReconnect(checked);
  });

  connectMenu->addSeparator();

  auto settingsMenu = menuBar()->addMenu("&Settings");
  auto unifiedSpeed = settingsMenu->addAction("&Unified Speed");
  unifiedSpeed->setCheckable(true);
  unifiedSpeed->setChecked(settings.getUnifiedSpeed());
  connect(unifiedSpeed, &QAction::triggered, [this](auto checked) {
    settings.setUnifiedSpeed(checked);
  });
  settingsMenu->addAction(unifiedSpeed);

  auto dataPath = settingsMenu->addAction("Set &Data Path (" + settings.getDataPath() + ")");
  connect(dataPath, &QAction::triggered, [this, dataPath](){
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter("CSV (*.csv)");
    dialog.setDefaultSuffix("csv");
    dialog.setWindowTitle("Select CSV Data File Path");
    auto dir = QFileInfo(settings.getDataPath()).absoluteDir();
    if (dir.exists()) {
      dialog.setDirectory(dir);
    }
    if (dialog.exec()){
      auto selected = dialog.selectedFiles();
      if (!selected.empty()) {
        auto path = selected[0];
        settings.setDataPath(path);
        dataPath->setText("Set &Data Path (" + path + ")");
        if(!path.isEmpty()) {
          stats.changePath(path);
          if (!stats.hasFile()) {
            QMessageBox::warning(this, "Failed to load statistics", "Failed to parse file " + settings.getDataPath());
          }
        }
      }
    }
  });

  auto useSystemTheme = settingsMenu->addAction("Use S&ystem Theme");
  useSystemTheme->setCheckable(true);
  useSystemTheme->setChecked(settings.getUseSystemTheme());
  connect(useSystemTheme, &QAction::triggered, [this](auto checked) {
    settings.setUseSystemTheme(checked);
  });

  settingsMenu->addSection("Show Statistics");
  auto statisticsGroup = new QActionGroup(this);
  auto showStatistics = settings.getShowStatistics();
  auto addStatisticsAction = [=](const char *label, int setting){
    auto action = settingsMenu->addAction(label);
    action->setCheckable(true);
    statisticsGroup->addAction(action);
    connect(action, &QAction::triggered, this, [=]() {
      settings.setShowStatistics(setting);
      updateStatsLabel();
    });
    if (setting == showStatistics) action->setChecked(true);
  };
  addStatisticsAction("&Today", Settings::TODAY);
  addStatisticsAction("&All time", Settings::ALL_TIME);
  addStatisticsAction("&Current run", Settings::CURRENT_RUN);

  statusLabel = new QLabel("Disconnected", this);
  statusBar()->addPermanentWidget(statusLabel);
  messageLabel = new QLabel("Scanning...", this);
  statusBar()->addWidget(messageLabel);
}

void MainWindow::setupTimer() {
  tickTimer = new QTimer(this);
  tickTimer->setInterval(1000);
  connect(tickTimer, &QTimer::timeout, this, &MainWindow::tick);
  tickTimer->start();

  // It seems the WalkingPad ignores commands if they come in too quickly
  // So we only send one command from the queue every 50 msec
  sendTimer = new QTimer(this);
  sendTimer->setInterval(50);
  connect(sendTimer, &QTimer::timeout, this, &MainWindow::handleSend);
  sendTimer->start();
}

void MainWindow::handleSend() {
  if (state != CONNECTED) return;
  if (!sendQueue.empty()) {
    service->writeCharacteristic(writeChar, sendQueue.front());
    //qDebug() << "SEND" << sendQueue.front().toHex();
    sendQueue.pop_front();
  }
}

void MainWindow::send(const QByteArray &msg){
  sendQueue.push_back(msg);
}

void MainWindow::startDiscovering() {
  deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
  deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(2000);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &MainWindow::scanFinished);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &MainWindow::scanFinished);
  connect(deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &MainWindow::scanError);
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
  deviceAction->setEnabled(connectionsEnabled);
  connect(deviceAction, &QAction::triggered, this, [this](){
    auto action = qobject_cast<QAction*>(sender());
    selectedDevice = devices[action->data().toInt()];
    connectDevice();
  });

  devices.append(device);
  for (auto a : connectActions) {
    connectMenu->removeAction(a);
  }

  connectActions.append(deviceAction);
  std::sort(connectActions.begin(), connectActions.end(), [](const auto a, const auto b) {
    return a->text() < b->text(); });
  connectMenu->addActions(connectActions);

  if (!autoConnected && settings.getAutoReconnect() && device.address().toString() == settings.getLastUUID()) {
    autoConnected = true;
    selectedDevice = devices.back();
    connectDevice();
  }
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
  state = CONNECTING;
  sendQueue.clear();
  setConnectActionEnabled(false);
  disconnectAction->setEnabled(true);
  showMessage("Connecting to " + selectedDevice.name() + " ...");
  if (!bleController) {
    delete bleController;
  }
  service = nullptr;
  serviceUuid.reset();
  bleController = QLowEnergyController::createCentral(selectedDevice, this);
  connect(bleController, &QLowEnergyController::serviceDiscovered, this, &MainWindow::serviceDiscovered);
  connect(bleController, &QLowEnergyController::discoveryFinished, this, &MainWindow::discoveryFinished, Qt::QueuedConnection);
  connect(bleController, &QLowEnergyController::connected, this, &MainWindow::connected);
  connect(bleController, &QLowEnergyController::disconnected, this, &MainWindow::disconnected);
  connect(bleController, &QLowEnergyController::errorOccurred, this, &MainWindow::connectError);
  bleController->connectToDevice();
}

void MainWindow::connected() {
  qDebug("Connected");
  settings.setLastUUID(selectedDevice.address().toString());
  bleController->discoverServices();
  setStatus("Connected (" + selectedDevice.name() + ")");
  showMessage("");
}

void MainWindow::disconnected() {
  qDebug("Disconnected");
  setConnectActionEnabled(true);
  setStatus("Disconnected");
  bleController->disconnectFromDevice();
  state = DISCONNECTED;
  centerWidget->setEnabled(false);
  disconnectAction->setEnabled(false);
  sendQueue.clear();
  showMessage("");
}

void MainWindow::serviceDiscovered(const QBluetoothUuid &gatt) {
  qDebug() << "Service discovered" << gatt;
  if (gatt.toUInt16() == 0xfe00) {
    serviceUuid = gatt;
  }
}

void MainWindow::serviceStateChanged(QLowEnergyService::ServiceState newState) {
  if (newState != QLowEnergyService::RemoteServiceDiscovered) return;
  connect(service, &QLowEnergyService::characteristicChanged, this, &MainWindow::characteristicChanged);
  auto readChar = service->characteristic(QBluetoothUuid((quint16)0xfe01));
  writeChar = service->characteristic(QBluetoothUuid((quint16)0xfe02));
  if (!readChar.isValid() || !writeChar.isValid()) {
    qCritical("Characteristics not found");
    handleInvalidService();
  }
  auto notifyConfig = readChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
  service->writeDescriptor(notifyConfig, QByteArray::fromHex("0100"));
  state = CONNECTED;
  centerWidget->setEnabled(true);
  queriedParams = false;
  retrievingRecords = false;
}

void MainWindow::sendStart() {
  if (modeButtons[Pad::MODE_SLEEP]->isChecked())
    send(Pad::setMode(Pad::MODE_MANUAL));
  send(Pad::start());
  relativeSetTime = QDateTime::currentMSecsSinceEpoch();
  relativeSetSpeed = startSpeedSlider->sliderPosition();
}

void MainWindow::characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value){
  if (c.uuid().toUInt16() != 0xfe01) return;
  //qDebug() << "Characteristic Changed" << c.uuid() << value.toHex(' ');
  using namespace Pad;
  auto parsed = parseMessage(value);
  if (auto info = std::get_if<Info>(&parsed)) {
    //qDebug("Info: state %u, speed %u, mode %u, time %u, distance %u, steps %u", info->state, info->speed, info->mode, info->time, info->distance, info->steps);
    auto time = QDateTime::currentMSecsSinceEpoch();
    if (info->mode < 3 && (time - setModeTime) > 1000) {
      modeButtons[info->mode]->setChecked(true);
    }
    if ((time - setSpeedTime) > 1000 && !speedSlider->isSliderDown()) {
      if (!speedSlider->isSliderDown())
        setSpeedWidgets(info->speed);
    }
    currentSpeed = info->speed;
    currentData.distance = info->distance * 10;
    currentData.duration = info->time;
    currentData.steps = info->steps;
    updateStatsLabel();
    auto s = info->state;
    startButton->setText(s == 0 || s == 5 ? "Start" : "Stop");
  } else if (auto params = std::get_if<Params>(&parsed)) {
    //qDebug("Params: goalType %u, goal %u, regulate %u, maxSpeed %u, startSpeed %u, startMode %u, sensitivity %u, display %x, lock %u, unit %u", params->goalType, params->goal, params->regulate, params->maxSpeed, params->startSpeed, params->startMode, params->sensitivity, params->display, params->lock, params->unit);
    queriedParams = true; // only query params once
    setStartSpeedWidgets(params->startSpeed);

    maxSpeed = params->maxSpeed;
    startSpeedSlider->setMaximum(maxSpeed);
    speedSlider->setMaximum(maxSpeed);
  } else if (auto record = std::get_if<Record>(&parsed)) {
    //qDebug("Record: onTime %u, startTime %u, duration %u, distance %u, steps %u, remaining %u", record->onTime, record->startTime, record->duration, record->distance, record->steps, record->remainingRecords);
    remainingRecords = record->remainingRecords;
    if (record->duration) {
      stats.addRecord(*record);
      retrievingRecords = true;
      retrievingRecordsTime = QDateTime::currentMSecsSinceEpoch();
      send(syncRecord(remainingRecords));
    } else {
      retrievingRecords = false;
    }
  } else {
    qWarning() << "Unknown Message" << value.toHex();
  }
}

void MainWindow::discoveryFinished() {
  qDebug() << "Discovery Finished";
  if (!serviceUuid || !(service = bleController->createServiceObject(*serviceUuid, bleController))){
    handleInvalidService();
    return;
  }

  Q_ASSERT(service->state() == QLowEnergyService::RemoteService);
  connect(service, &QLowEnergyService::stateChanged,this, &MainWindow::serviceStateChanged);
  connect(service, &QLowEnergyService::errorOccurred,this, [](auto x){qWarning() << "Error while discovering details:" << x;});
  service->discoverDetails();
}

void MainWindow::connectError(QLowEnergyController::Error error) {
  auto err {bleController->errorString()};
  qCritical() << "Error while connecting to " << selectedDevice.name() << err;
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
  send(Pad::query());
  if (!queriedParams)
    send(Pad::queryParams());
  if (retrievingRecords && (QDateTime::currentMSecsSinceEpoch() - retrievingRecordsTime > 5000))
    retrievingRecords = false;
  if (!retrievingRecords)
    send(Pad::syncRecord(255));
}

void MainWindow::setConnectActionEnabled(bool enabled) {
  connectionsEnabled = enabled;
  for (auto a : connectActions) {
    a->setEnabled(enabled);
  }
}

void MainWindow::handleInvalidService() {
  qCritical("No WalkingPad Service found");
  QMessageBox::warning(this, "No WalkingPad Service", selectedDevice.name() + " does not appear to be a WalkingPad. Disconnecting...");
  showMessage("No WalkingPad Service. Disconnecting...");
  bleController->disconnectFromDevice();
}

void MainWindow::updateStatsLabel() {
  auto dat = currentData;
  auto addStats = settings.getShowStatistics();
  if (addStats == Settings::ALL_TIME) {
    dat += stats.allTime();
  } else if (addStats == Settings::TODAY) {
    dat += stats.today();
  }
  constexpr auto w = 8;
  statsLabel->setText(QString("%1  km/h\n"
                              "%2  time\n"
                              "%3  steps\n"
                              "%4  km")
    .arg(currentSpeed/10., w, 'f', 1)
    .arg(QTime::fromMSecsSinceStartOfDay(dat.duration * 1000).toString("hh:mm:ss"), w)
    .arg(dat.steps, w)
    .arg(dat.distance/1000., w, 'f', 2));
}

void MainWindow::receivedMessage(int instanceId, QByteArray message) {
  if (state != CONNECTED) return;
  auto args = message.split(' ');
  if (args.empty()) return;
  auto cmd = args[0];
  if (cmd == "start") {
    sendStart();
  } else {
    if (args.size() < 2) return;
    bool ok;
    int val = args[1].toInt(&ok);
    if (!ok) return;
    if (cmd == "setSpeed" && val >= 0 && val <= maxSpeed) {
      send(Pad::setSpeed(val));
    } else if (cmd == "addSpeed") {
      addSpeed(val);
    }
  }
}

void MainWindow::addSpeed(int s) {
  auto now = QDateTime::currentMSecsSinceEpoch();
  if ((now - relativeSetTime) > 5000) {
    relativeSetSpeed = currentSpeed;
  }
  auto speed = relativeSetSpeed + s;
  if (speed >= 0 && speed <= maxSpeed) {
    relativeSetSpeed = speed;
    send(Pad::setSpeed(speed));
  }
  relativeSetTime = now;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  disconnect();
}

void MainWindow::setupHotkeys() {
  // TODO make hotkeys configurable
  auto startKey = new QHotkey(QKeySequence("Meta+\\"), true, this);
  connect(startKey, &QHotkey::activated, [=]{ if(state == CONNECTED) sendStart(); });
  constexpr int delta = 5;
  auto fasterKey = new QHotkey(QKeySequence("Meta+]"), true, this);
  connect(fasterKey, &QHotkey::activated, [=]{ if(state == CONNECTED) addSpeed(delta); });
  auto slowerKey = new QHotkey(QKeySequence("Meta+["), true, this);
  connect(slowerKey, &QHotkey::activated, [=]{ if(state == CONNECTED) addSpeed(-delta); });
}
