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
  enum Statistics {
    TODAY, ALL_TIME, CURRENT_RUN
  };
  SETTING(bool, AutoReconnect, true)
  SETTING(bool, UnifiedSpeed, true)
  SETTING(bool, UseSystemTheme, true)
  SETTING(QString, LastUUID, "")
  SETTING(QString, DataPath, "")
  SETTING(int, ShowStatistics, TODAY)
private:
  QSettings settings;
};

#undef SETTING