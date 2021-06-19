#pragma once

#include <QMainWindow>
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include "Settings.h"

class QBluetoothDeviceInfo;
class QLabel;
class QLowEnergyController;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();

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

  QLowEnergyController *bleController {};
  QLowEnergyService *service {};
  QLowEnergyCharacteristic writeChar;

  QTimer *timer;

  Settings settings;

  bool autoConnected {}; //only auto connect once
  bool connectionsEnabled {true};
};

