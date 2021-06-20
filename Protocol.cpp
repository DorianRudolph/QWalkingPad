#include "Protocol.h"

namespace Pad {

QByteArray messageByte(uint8_t k, uint8_t v) {
  uint8_t data[]{0xf7, 0xa2, k, v, uint8_t(0xa2 + k + v), 0xfd};
  return QByteArray(reinterpret_cast<char *>(data), sizeof(data));
}

QByteArray messageInt(uint8_t k, uint32_t v) {
  uint8_t data[]{0xf7, 0xa6, k, 0, uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v), 0, 0xfd};
  for (int i = 1; i < 7; ++i) data[7] += data[i];
  return QByteArray(reinterpret_cast<char *>(data), sizeof(data));
}

QByteArray query() {
  return messageByte(0, 0);
}

QByteArray queryParams() {
  return messageInt(0, 0);
}

QByteArray setSpeed(uint8_t speed) {
  return messageByte(1, speed);
}

QByteArray setStartSpeed(uint8_t speed) {
  return messageInt(4, speed);
}

QByteArray setMode(uint8_t mode) {
  return messageByte(2, mode);
}

QByteArray start() {
  return messageByte(4, 1);
}

QByteArray syncRecord(uint8_t n) {
  uint8_t data[]{0xf7, 0xa7, 0xaa, n, uint8_t(0xa7 + 0xaa + n), 0xfd};
  return QByteArray(reinterpret_cast<char *>(data), sizeof(data));
}

QByteArray setCali(uint8_t enable) {
  return messageInt(2, enable);
}

QByteArray setMaxSpeed(uint8_t speed) {
  return messageInt(3, speed);
}

QByteArray setAutoStart(uint8_t enable) {
  return messageInt(5, enable);
}

QByteArray setSensitivity(uint8_t sensitivity) {
  return messageInt(6, sensitivity);
}

QByteArray setUnit(uint8_t unit) {
  return messageInt(8, unit);
}

QByteArray setLock(uint8_t enable) {
  return messageInt(9, enable);
}

QByteArray setDisplayInfo(uint8_t info) {
  return messageInt(7, info);
}

static uint32_t parseInt(const uint8_t *m, int i) {
  return (m[i] << 16) | (m[i + 1] << 8) | m[i + 2];
}

Message parseMessage(const QByteArray &message) {
  Message parsed;
  auto m = reinterpret_cast<const uint8_t *>(message.constData());
  auto sz = message.size();
  auto typ = m[1];
  if (typ == 0xa2 && sz >= 15) {
    parsed = Info{
        .state = uint8_t(m[2]),
        .speed = uint8_t(m[3]),
        .mode = uint8_t(m[4]),
        .time = parseInt(m, 5),
        .distance = parseInt(m, 8),
        .steps = parseInt(m, 11),
    };
  } else if (typ == 0xa6 && sz >= 14) {
    parsed = Params{
        .goalType = m[2],
        .goal = parseInt(m, 3),
        .regulate = m[6],
        .maxSpeed = m[7],
        .startSpeed = m[8],
        .startMode = m[9],
        .sensitivity = m[10],
        .display = m[11],
        .lock = m[12],
        .unit = m[13],
    };
  } else if (typ == 0xa7 && sz >= 18) {
    parsed = Record{
        .onTime = parseInt(m, 2),
        .startTime = parseInt(m, 5),
        .duration = parseInt(m, 8),
        .distance = parseInt(m, 11),
        .steps = parseInt(m, 14),
        .remainingRecords = m[17]
    };
  }
  return parsed;
}

}