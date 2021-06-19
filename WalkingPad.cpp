#include "WalkingPad.h"

QByteArray messageByte(uint8_t k, uint8_t v) {
  uint8_t data[] {0xf7, 0xa2, k, v, uint8_t(0xa2 + k + v), 0xfd};
  return QByteArray(reinterpret_cast<char*>(data), sizeof(data));
}

QByteArray messageInt(uint8_t k, uint32_t v) {
  uint8_t data[] {0xf7, 0xa6, k, 0, uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v), 0, 0xfd};
  for (int i = 1; i < 7; ++i) data[7] += data[i];
  return QByteArray(reinterpret_cast<char*>(data), sizeof(data));
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
  uint8_t data[] {0xf7, 0xa7, 0xaa, n, uint8_t(0xa7 + 0xaa + n), 0xfd};
  return QByteArray(reinterpret_cast<char*>(data), sizeof(data));
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
