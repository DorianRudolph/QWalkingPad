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
#include <QApplication>
#include <QCloseEvent>
#include <QIcon>
#include <QKeySequence>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

namespace {
QIcon makeTrayIcon(bool walking) {
  QPixmap pixmap(64, 64);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(walking ? QColor(22, 163, 74) : QColor(75, 85, 99));
  painter.setPen(Qt::NoPen);
  painter.drawRoundedRect(6, 6, 52, 52, 10, 10);
  painter.setPen(Qt::white);
  auto font = painter.font();
  font.setBold(true);
  font.setPixelSize(walking ? 28 : 22);
  painter.setFont(font);
  painter.drawText(pixmap.rect(), Qt::AlignCenter, walking ? ">" : "WP");
  return QIcon(pixmap);
}
}

MainWindow::MainWindow() {
  setWindowTitle("QWalkingPad");
  setupLayout();
  setupMenu();
  setupTrayIcon();
  setupTimer();
  startDiscovering();
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (quitting || !trayIcon || !trayIcon->isVisible()) {
    QMainWindow::closeEvent(event);
    return;
  }

  hide();
  event->ignore();
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
      setMode(mode);
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
    if (checked && state == DISCONNECTED) {
      autoReconnectPaused = false;
      scheduleReconnect();
    } else if (!checked && reconnectTimer) {
      reconnectTimer->stop();
    }
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

  auto quitAction = new QAction("&Quit", this);
  quitAction->setShortcut(QKeySequence::Quit);
  quitAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(quitAction);
  connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
}

void MainWindow::setupTrayIcon() {
  if (!QSystemTrayIcon::isSystemTrayAvailable())
    return;

  trayIcon = new QSystemTrayIcon(makeTrayIcon(false), this);
  trayIcon->setToolTip("QWalkingPad");

  trayMenu = new QMenu(this);
  auto showAction = trayMenu->addAction("&Show QWalkingPad");
  connect(showAction, &QAction::triggered, this, &MainWindow::showWindow);
  trayMenu->addSeparator();

  trayStatusAction = trayMenu->addAction("Disconnected");
  trayStatusAction->setEnabled(false);
  trayStartAction = trayMenu->addAction("&Start");
  connect(trayStartAction, &QAction::triggered, this, &MainWindow::sendStart);

  traySlowerAction = trayMenu->addAction("Slower (-0.5 km/h)");
  connect(traySlowerAction, &QAction::triggered, this, [this] {
    addSpeed(-5);
  });
  trayFasterAction = trayMenu->addAction("Faster (+0.5 km/h)");
  connect(trayFasterAction, &QAction::triggered, this, [this] {
    addSpeed(5);
  });

  trayMenu->addSeparator();
  auto modeGroup = new QActionGroup(this);
  traySleepAction = trayMenu->addAction("&Sleep");
  trayManualAction = trayMenu->addAction("&Manual");
  trayAutoAction = trayMenu->addAction("&Auto");
  for (auto action : {traySleepAction, trayManualAction, trayAutoAction}) {
    action->setCheckable(true);
    modeGroup->addAction(action);
  }
  connect(traySleepAction, &QAction::triggered, this, [this] {
    setMode(Pad::MODE_SLEEP);
  });
  connect(trayManualAction, &QAction::triggered, this, [this] {
    setMode(Pad::MODE_MANUAL);
  });
  connect(trayAutoAction, &QAction::triggered, this, [this] {
    setMode(Pad::MODE_AUTO);
  });

  trayMenu->addSeparator();
  trayDisconnectAction = trayMenu->addAction("&Disconnect");
  connect(trayDisconnectAction, &QAction::triggered, this, [this] {
    if (state == CONNECTED || state == CONNECTING) {
      disconnect();
    } else {
      autoReconnectPaused = false;
      reconnectRequested = true;
      for (const auto &device : devices) {
        if (device.address().toString() == settings.getLastUUID()) {
          selectedDevice = device;
          connectDevice(true);
          return;
        }
      }
      showMessage("Scanning for WalkingPad...");
      startScan(false);
    }
  });

  trayMenu->addSeparator();
  auto quitAction = trayMenu->addAction("&Quit");
  connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);

  connect(trayIcon, &QSystemTrayIcon::activated, this, [this](auto reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
      showWindow();
    }
  });

  trayIcon->setContextMenu(trayMenu);
  trayIcon->show();
  updateTrayActions();
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

  reconnectTimer = new QTimer(this);
  reconnectTimer->setSingleShot(true);
  reconnectTimer->setInterval(5000);
  connect(reconnectTimer, &QTimer::timeout, this, &MainWindow::retryReconnect);
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
  startScan(false);
}

void MainWindow::disconnect() {
  if (!bleController)
    return;
  reconnectTimer->stop();
  autoReconnectPaused = true;
  reconnectRequested = false;
  state = DISCONNECTING;
  bleController->disconnectFromDevice();
  showMessage("Disconnecting...");
  if (bleController->state() != QLowEnergyController::ClosingState){
    disconnected();
  }
  updateTrayActions();
}

void MainWindow::addDevice(const QBluetoothDeviceInfo &device) {
  if (!(device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
    return;
  qDebug() << "Found" << device.name() << device.address();

  auto deviceAddress = device.address().toString();
  for (auto i = 0; i < devices.size(); ++i) {
    if (devices[i].address().toString() == deviceAddress) {
      devices[i] = device;
      if (state == DISCONNECTED && (shouldAutoReconnect() || reconnectRequested) && deviceAddress == settings.getLastUUID()) {
        selectedDevice = device;
        connectDevice(true);
      }
      return;
    }
  }

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

  if (state == DISCONNECTED && (shouldAutoReconnect() || reconnectRequested) && deviceAddress == settings.getLastUUID()) {
    selectedDevice = devices.back();
    connectDevice(true);
  }
}

void MainWindow::scanFinished() {
  if (state == DISCONNECTED){
    if (shouldAutoReconnect()) {
      showMessage("Waiting for WalkingPad...");
      scheduleReconnect();
    } else if (reconnectRequested) {
      reconnectRequested = false;
      showMessage("WalkingPad not found");
    } else {
      showMessage("Scan finished");
    }
  }
  scanAction->setEnabled(state == DISCONNECTED);
  updateTrayActions();
}

void MainWindow::scanError(QBluetoothDeviceDiscoveryAgent::Error error) {
  auto err = deviceDiscoveryAgent->errorString();
  qCritical() << "Error while scanning for devices" << err;
  showMessage("Scan error: " + err);
  if (state == DISCONNECTED && shouldAutoReconnect()) {
    scheduleReconnect();
  }
}

void MainWindow::scan() {
  reconnectTimer->stop();
  startScan(true);
}

void MainWindow::startScan(bool clearExistingDevices) {
  if (clearExistingDevices) {
    clearDevices();
  }
  if (deviceDiscoveryAgent->isActive()) {
    deviceDiscoveryAgent->stop();
  }
  scanAction->setDisabled(true);
  deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void MainWindow::clearDevices() {
  for (auto a : connectActions) {
    connectMenu->removeAction(a);
    delete a;
  }
  devices.clear();
  connectActions.clear();
}

void MainWindow::scheduleReconnect() {
  if ((!shouldAutoReconnect() && !reconnectRequested) || state != DISCONNECTED)
    return;
  if (!reconnectTimer->isActive())
    reconnectTimer->start();
}

bool MainWindow::shouldAutoReconnect() {
  return !autoReconnectPaused && settings.getAutoReconnect() && !settings.getLastUUID().isEmpty();
}

void MainWindow::retryReconnect() {
  if (state != DISCONNECTED || (!shouldAutoReconnect() && !reconnectRequested))
    return;

  showMessage("Scanning for WalkingPad...");
  startScan(false);
}

void MainWindow::connectDevice(bool automaticReconnect) {
  if (state == CONNECTING || state == CONNECTED)
    return;

  state = CONNECTING;
  autoReconnectPaused = false;
  reconnectingAutomatically = automaticReconnect;
  reconnectTimer->stop();
  if (deviceDiscoveryAgent->isActive()) {
    deviceDiscoveryAgent->stop();
  }
  sendQueue.clear();
  setConnectActionEnabled(false);
  scanAction->setDisabled(true);
  disconnectAction->setEnabled(true);
  showMessage("Connecting to " + selectedDevice.name() + " ...");
  if (bleController) {
    retireController(bleController);
    bleController = nullptr;
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

void MainWindow::retireController(QLowEnergyController *controller) {
  if (!controller)
    return;

  QObject::disconnect(controller, nullptr, this, nullptr);
  controller->setParent(nullptr);

  if (controller->state() == QLowEnergyController::UnconnectedState) {
    controller->deleteLater();
    return;
  }

  connect(controller, &QLowEnergyController::disconnected, controller, &QObject::deleteLater);
  controller->disconnectFromDevice();
}

void MainWindow::connected() {
  if (sender() != bleController)
    return;

  qDebug("Connected");
  settings.setLastUUID(selectedDevice.address().toString());
  bleController->discoverServices();
  setStatus("Connected (" + selectedDevice.name() + ")");
  showMessage("");
  updateTrayActions();
}

void MainWindow::disconnected() {
  if (sender() && sender() != bleController)
    return;

  qDebug("Disconnected");
  auto reconnect = state != DISCONNECTING && (shouldAutoReconnect() || reconnectRequested);
  setConnectActionEnabled(true);
  setStatus("Disconnected");
  state = DISCONNECTED;
  reconnectingAutomatically = false;
  service = nullptr;
  serviceUuid.reset();
  centerWidget->setEnabled(false);
  disconnectAction->setEnabled(false);
  scanAction->setEnabled(true);
  sendQueue.clear();
  if (reconnect) {
    showMessage("Disconnected. Reconnecting...");
    scheduleReconnect();
  } else {
    showMessage("");
  }
  updateTrayActions();
}

void MainWindow::serviceDiscovered(const QBluetoothUuid &gatt) {
  if (sender() != bleController)
    return;

  qDebug() << "Service discovered" << gatt;
  if (gatt.toUInt16() == 0xfe00) {
    serviceUuid = gatt;
  }
}

void MainWindow::serviceStateChanged(QLowEnergyService::ServiceState newState) {
  if (sender() != service)
    return;

  if (newState != QLowEnergyService::RemoteServiceDiscovered) return;
  connect(service, &QLowEnergyService::characteristicChanged, this, &MainWindow::characteristicChanged);
  auto readChar = service->characteristic(QBluetoothUuid((quint16)0xfe01));
  writeChar = service->characteristic(QBluetoothUuid((quint16)0xfe02));
  if (!readChar.isValid() || !writeChar.isValid()) {
    qCritical("Characteristics not found");
    handleInvalidService();
    return;
  }
  auto notifyConfig = readChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
  service->writeDescriptor(notifyConfig, QByteArray::fromHex("0100"));
  state = CONNECTED;
  reconnectRequested = false;
  reconnectingAutomatically = false;
  centerWidget->setEnabled(true);
  queriedParams = false;
  retrievingRecords = false;
  updateTrayActions();
}

void MainWindow::sendStart() {
  if (state != CONNECTED)
    return;
  if (modeButtons[Pad::MODE_SLEEP]->isChecked())
    send(Pad::setMode(Pad::MODE_MANUAL));
  send(Pad::start());
  relativeSetTime = QDateTime::currentMSecsSinceEpoch();
  relativeSetSpeed = startSpeedSlider->sliderPosition();
}

void MainWindow::setMode(int mode) {
  if (state != CONNECTED)
    return;

  setModeTime = QDateTime::currentMSecsSinceEpoch();
  currentMode = mode;
  send(Pad::setMode(static_cast<uint8_t>(mode)));
  updateTrayActions();
}

void MainWindow::addSpeed(int speedDelta) {
  if (state != CONNECTED)
    return;

  auto now = QDateTime::currentMSecsSinceEpoch();
  if ((now - relativeSetTime) > 5000) {
    relativeSetSpeed = currentSpeed;
  }
  auto speed = relativeSetSpeed + speedDelta;
  if (speed >= 0 && speed <= maxSpeed) {
    relativeSetSpeed = speed;
    send(Pad::setSpeed(speed));
  }
  relativeSetTime = now;
}

void MainWindow::characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value){
  if (sender() != service)
    return;

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
    currentMode = info->mode;
    currentData.distance = info->distance * 10;
    currentData.duration = info->time;
    currentData.steps = info->steps;
    updateStatsLabel();
    auto s = info->state;
    padRunning = s != 0 && s != 5;
    startButton->setText(padRunning ? "Stop" : "Start");
    updateTrayActions();
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
  if (sender() != bleController)
    return;

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
  if (sender() != bleController)
    return;

  auto err {bleController->errorString()};
  qCritical() << "Error while connecting to " << selectedDevice.name() << err;
  showMessage("Connection error: " + err);
  if (state != DISCONNECTING && !reconnectingAutomatically) {
    QMessageBox::warning(this, "Connection Error", "Error while connecting: " + err + " ");
  }
  disconnected();
}

void MainWindow::showMessage(const QString &message) {
  messageLabel->setText(message);
}

void MainWindow::setStatus(const QString &status) {
  statusLabel->setText(status);
  updateTrayActions();
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
  if (!reconnectingAutomatically) {
    QMessageBox::warning(this, "No WalkingPad Service", selectedDevice.name() + " does not appear to be a WalkingPad. Disconnecting...");
  }
  showMessage("No WalkingPad Service. Disconnecting...");
  if (bleController)
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

void MainWindow::updateTrayActions() {
  if (!trayIcon)
    return;

  auto connected = state == CONNECTED;
  auto canControl = connected && service;
  auto speedText = QString("%1 km/h").arg(currentSpeed/10., 0, 'f', 1);
  auto status = connected ? QString("Connected: %1").arg(speedText) : statusLabel->text();
  trayStatusAction->setText(status);
  trayIcon->setToolTip("QWalkingPad - " + status);
  if (trayIconWalkingState != padRunning) {
    trayIconWalkingState = padRunning;
    trayIcon->setIcon(makeTrayIcon(padRunning));
  }

  trayStartAction->setText(padRunning ? "&Stop" : "&Start");
  for (auto action : {trayStartAction, traySlowerAction, trayFasterAction,
       traySleepAction, trayManualAction, trayAutoAction}) {
    action->setEnabled(canControl);
  }

  traySleepAction->setChecked(currentMode == Pad::MODE_SLEEP);
  trayManualAction->setChecked(currentMode == Pad::MODE_MANUAL);
  trayAutoAction->setChecked(currentMode == Pad::MODE_AUTO);

  if (state == CONNECTED || state == CONNECTING) {
    trayDisconnectAction->setText("&Disconnect");
    trayDisconnectAction->setEnabled(bleController);
  } else {
    trayDisconnectAction->setText("&Reconnect");
    trayDisconnectAction->setEnabled(!settings.getLastUUID().isEmpty());
  }
}

void MainWindow::showWindow() {
  show();
  raise();
  activateWindow();
}

void MainWindow::quitApplication() {
  quitting = true;
  if (trayIcon)
    trayIcon->hide();
  if (!bleController || bleController->state() == QLowEnergyController::UnconnectedState) {
    QApplication::quit();
    return;
  }

  reconnectTimer->stop();
  state = DISCONNECTING;
  auto controller = bleController;
  bleController = nullptr;
  service = nullptr;
  QObject::disconnect(controller, nullptr, this, nullptr);
  controller->setParent(nullptr);
  connect(controller, &QLowEnergyController::disconnected, controller, &QObject::deleteLater);
  connect(controller, &QLowEnergyController::disconnected, qApp, &QApplication::quit);
  controller->disconnectFromDevice();
  QTimer::singleShot(3000, qApp, &QApplication::quit);
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
