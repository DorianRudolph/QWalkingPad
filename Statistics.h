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


