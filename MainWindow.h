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

#pragma once

#include <QMainWindow>
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include "Settings.h"
#include "Statistics.h"

class QBluetoothDeviceInfo;
class QLabel;
class QLowEnergyController;
class QRadioButton;
class QSlider;
class QPushButton;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();

public slots:
  void receivedMessage(int instanceId, QByteArray message);

protected:
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private slots:
  void disconnect();
  void addDevice(const QBluetoothDeviceInfo &device);
  void scanFinished();
  void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
  void tick();
  void serviceDiscovered(const QBluetoothUuid &gatt);
  void discoveryFinished();
  void connectError(QLowEnergyController::Error error);
  void connected();
  void disconnected();
  void serviceStateChanged(QLowEnergyService::ServiceState newState);
  void characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
  void handleSend();
  void setSpeedWidgets(int speed);
  void setStartSpeedWidgets(int speed);

private:
  void startDiscovering();
  void setupMenu();
  void scan();
  void showMessage(const QString &message);
  void setStatus(const QString &status);
  void setupTimer();
  void setConnectActionEnabled(bool enabled);
  void handleInvalidService();
  void connectDevice();
  void setupLayout();
  void send(const QByteArray &msg);
  QSlider *makeSpeedSlider();
  void updateStatsLabel();
  void sendStart();
  void setupHotkeys();
  void addSpeed(int s);

  QLabel *statusLabel;
  QLabel *messageLabel;

  enum {
    DISCONNECTED,
    DISCONNECTING,
    CONNECTING,
    CONNECTED
  } state {DISCONNECTED};
  QMenu *connectMenu;
  QAction *disconnectAction;
  QAction *scanAction;
  QList<QAction*> connectActions;
  QList<QBluetoothDeviceInfo> devices;
  QBluetoothDeviceInfo selectedDevice;
  QBluetoothDeviceDiscoveryAgent *deviceDiscoveryAgent {};

  std::optional<QBluetoothUuid> serviceUuid {};

  QList<QRadioButton*> modeButtons {nullptr, nullptr, nullptr};

  QLowEnergyController *bleController {};
  QLowEnergyService *service {};
  QLowEnergyCharacteristic writeChar;

  QTimer *tickTimer;
  QTimer *sendTimer;
  QList<QByteArray> sendQueue;

  Settings settings;

  bool autoConnected {}; //only auto connect once
  bool connectionsEnabled {true};

  qint64 setModeTime {}; // don't update UI directly after setting
  qint64 setSpeedTime {};

  QWidget *centerWidget;

  QSlider *speedSlider;
  QLabel *speedLabel;
  QSlider *startSpeedSlider;
  QLabel *startSpeedLabel;
  QPushButton *startButton;

  bool queriedParams {};

  Pad::Statistics::Data currentData {};
  Pad::Statistics stats {};
  int currentSpeed {};
  QLabel *statsLabel;

  bool retrievingRecords {};
  uint8_t remainingRecords {};
  qint64 retrievingRecordsTime {};

  int relativeSetSpeed;
  qint64 relativeSetTime {};

  int maxSpeed {60};
};

