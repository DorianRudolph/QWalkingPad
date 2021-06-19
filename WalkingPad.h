#pragma once
#include <QByteArray>

constexpr const uint8_t MODE_AUTO = 0;
constexpr const uint8_t MODE_MANUAL = 1;
constexpr const uint8_t MODE_SLEEP = 2;

constexpr const uint8_t SENSITIVITY_HIGH = 1;
constexpr const uint8_t SENSITIVITY_MEDIUM = 2;
constexpr const uint8_t SENSITIVITY_LOW = 3;

constexpr const uint8_t UNIT_METRIC = 0;
constexpr const uint8_t UNIT_IMPERIAL = 1;

constexpr const uint8_t DISPLAY_TIME = 0b1;
constexpr const uint8_t DISPLAY_SPEED = 0b10;
constexpr const uint8_t DISPLAY_DISTANCE = 0b100;
constexpr const uint8_t DISPLAY_CALORIE = 0b1000;
constexpr const uint8_t DISPLAY_STEP = 0b10000;

QByteArray messageByte(uint8_t k, uint8_t v);
QByteArray messageInt(uint8_t k, uint32_t v);
QByteArray query();
QByteArray queryParams();
QByteArray setSpeed(uint8_t speed); // hm/h
QByteArray setStartSpeed(uint8_t speed);
QByteArray setMode(uint8_t mode);
QByteArray start();
QByteArray syncRecord(uint8_t n); // 255 gets latest record, then use "remaining" to pop the record
QByteArray setCali(uint8_t enable); // 1 enables calibration mode, 0 disables it but keeps pad running
QByteArray setMaxSpeed(uint8_t speed);
QByteArray setAutoStart(uint8_t enable); // start when stepping onto pad
QByteArray setSensitivity(uint8_t sensitivity);
QByteArray setUnit(uint8_t unit);
QByteArray setLock(uint8_t enable);
QByteArray setDisplayInfo(uint8_t info);

inline constexpr unsigned char operator "" _uchar( unsigned long long arg ) noexcept {
  return static_cast< unsigned char >( arg );
}
