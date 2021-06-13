#pragma once

#include <QMainWindow>
#include <QBluetoothDeviceInfo>

class QBluetoothDeviceDiscoveryAgent;
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
  void connectDevice();

private:
  void startDiscovering();
  void setupMenu();
  void scan();

  QLabel *statusLabel;

  bool connected {};
  QMenu *connectMenu;
  QAction *disconnectAction;
  QAction *scanAction;
  QList<QAction*> connectActions;
  QList<QBluetoothDeviceInfo> devices;
  QBluetoothDeviceDiscoveryAgent *deviceDiscoveryAgent;

  QLowEnergyController *bleController {};
};

