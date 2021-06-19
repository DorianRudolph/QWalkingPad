#pragma once

#include <QSettings>
#include <QString>

#define SETTING(type, name, default) \
  type get##name() { \
    return settings.value(#name, default).value<type>(); \
  } \
  void set##name(const type &val) { \
    settings.setValue(#name, val);              \
    settings.sync();                          \
  }
class Settings {
public:
  SETTING(bool, AutoReconnect, true)
  SETTING(bool, UnifiedSpeed, true)
  SETTING(bool, UseSystemTheme, true)
  SETTING(QString, LastUUID, "")
  SETTING(QString, DataPath, "")
private:
  QSettings settings;
};

#undef SETTING