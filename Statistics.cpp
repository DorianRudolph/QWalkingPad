#include "Statistics.h"
#include <QLoggingCategory>
#include "Protocol.h"

namespace Pad {

Statistics::Data Statistics::Data::operator+(const Statistics::Data &other) const {
  return Statistics::Data{
      .duration = duration + other.duration,
      .distance = distance + other.distance,
      .steps = steps + other.steps,
  };
}

Statistics::Data &Statistics::Data::operator+=(const Statistics::Data &rhs) {
  duration += rhs.duration;
  distance += rhs.distance;
  steps += rhs.steps;
  return *this;
}

Statistics::Data Statistics::today() {
  Data dat{};
  auto today = QDate::currentDate();
  for (const auto &stat : stats) {
    if (stat.date.date() == today) {
      dat += stat.data;
    }
  }
  return dat;
}

static bool parseStat(const QByteArray &line, Statistics::Stat &stat) {
  auto split = line.trimmed().split(',');
  if (split.size() != 4 || !(stat.date = QDateTime::fromString(split[0].trimmed(), Qt::ISODate)).isValid()) {
    return false;
  }

  bool ok1, ok2, ok3;
  stat.data.duration = split[1].toInt(&ok1);
  stat.data.distance = split[2].toInt(&ok2);
  stat.data.steps = split[3].toInt(&ok3);

  return ok1 && ok2 && ok3;
}

static QByteArray stringifyStat(const Statistics::Stat &stat) {
  QByteArray b{stat.date.toString(Qt::ISODate).toUtf8()};
  b.push_back(',');
  b.push_back(QByteArray::number(stat.data.duration));
  b.push_back(',');
  b.push_back(QByteArray::number(stat.data.distance));
  b.push_back(',');
  b.push_back(QByteArray::number(stat.data.steps));
  b.push_back('\n');
  return b;
}

constexpr auto HEADER = "Date,Duration,Distance,Steps\n";

void Statistics::load(const QString &path) {
  file = std::make_unique<QFile>(path);
  if (!file->open(QFile::ReadWrite | QFile::Text)) {
    qWarning() << "Failed to open file" << path;
    file = nullptr;
    return;
  }

  int lineNo{};

  while (!file->atEnd()) {
    auto line = file->readLine();
    if (line.back() != '\n') {
      file->write("\n");
      line.append('\n');
    }
    if (line == "\n") continue;
    if (lineNo++ == 0) {
      if (line != HEADER) goto readError;
    } else {
      Stat stat;
      if (parseStat(line, stat)) {
        addStat(stat);
      } else {
        goto readError;
      }
    }
  }
  if (lineNo) return;

  readError:
  stats.clear();
  file->resize(0);
  file->write(HEADER);
}

bool Statistics::hasFile() {
  return !!file;
}

void Statistics::addStat(const Stat &stat) {
  stats.push_back(stat);
  if (file != nullptr) {
    writeStat(stat);
  }
}

void Statistics::addRecord(const Record &record) {
  addStat(Stat::fromRecord(record));
}

void Statistics::changePath(const QString &path) {
  file = std::make_unique<QFile>(path);
  if (!file->open(QFile::WriteOnly | QFile::Text)) {
    qWarning() << "Failed to open file" << path;
    file = nullptr;
    return;
  }
  file->write(HEADER);
  for (const auto &stat : stats) {
    writeStat(stat);
  }
}

void Statistics::writeStat(const Statistics::Stat &stat) {
  file->write(stringifyStat(stat));
  file->flush();
}

Statistics::Stat Statistics::Stat::fromRecord(const Record &record) {
  return {
      .date = QDateTime::currentDateTime().addMSecs((qint64)record.startTime - (qint64)record.onTime),
      .data = {
          .duration = static_cast<int>(record.duration),
          .distance = static_cast<int>(record.distance * 10),
          .steps = static_cast<int>(record.steps),
      }
  };
}
}
