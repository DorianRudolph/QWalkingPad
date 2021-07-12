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

