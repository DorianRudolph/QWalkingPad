#pragma once

#include <QMainWindow>
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include "Settings.h"

class QBluetoothDeviceInfo;
class QLabel;
class QLowEnergyController;
class QRadioButton;
class QSlider;

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
  void handleSend();

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
  void setSpeedWidgets(int speed);
  void setStartSpeedWidgets(int speed);
  QSlider *makeSpeedSlider();

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
};

