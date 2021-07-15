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
#include <QDateTime>
#include <QList>
#include <QFile>
#include <memory>

class QString;

namespace Pad {

class Record;

class Statistics {
public:
  struct Data {
    int duration;  //seconds
    int distance; //meters
    int steps;

    Data operator+(const Data &other) const;
    Data& operator+=(const Data& rhs);
  };

  struct Stat {
    QDateTime date;
    Data data;
    static Stat fromRecord(const Record &record);
  };

  void addRecord(const Record &record);
  void load(const QString &path);
  void changePath(const QString &path);
  Data today();
  Data allTime();
  bool hasFile();

private:
  void addStat(const Stat &stat);
  void writeStat(const Stat &stat);

  QList<Stat> stats;
  std::unique_ptr<QFile> file;
};
}


