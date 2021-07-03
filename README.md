# QWalkingPad

Simple desktop application for controlling the [Kingsmith WalkingpPad](https://walkingpad.com) over BLE (Bluetooth Low Energy).

![](screenshot.png)


- Control from command line while the application is running:
  - `qwalkingpad start`
  - `qwalkingpad addSpeed <speed>` (speed in 0.1 km/h)
  - `qwalkingpad setSpeed <speed>`
- Track Statistics in CSV file.
- Set speed in 0.1 km/h increments (not possible with the [app](https://play.google.com/store/apps/details?id=com.walkingpad.app) or remote)
- (Maybe) cross-platform, written in C++ with Qt (only tested on Arch Linux with the WalkingPad A1 Pro).
- *eternal alpha*, contributions welcome