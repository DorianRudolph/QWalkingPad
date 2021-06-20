#pragma once
#include <QByteArray>
#include <variant>

namespace Pad {

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
QByteArray setSpeed(uint8_t speed); // 0.1 km/h
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

struct Info {
  uint8_t state;
  uint8_t speed;
  uint8_t mode;
  uint32_t time;
  uint32_t distance; // 10 m
  uint32_t steps;
};

struct Params {
  uint8_t goalType;
  uint32_t goal;
  uint8_t regulate; // calibrate?
  uint8_t maxSpeed;
  uint8_t startSpeed;
  uint8_t startMode; // autoStart?
  uint8_t sensitivity;
  uint8_t display;
  uint8_t lock;
  uint8_t unit;
};

struct Record {
  uint32_t onTime;
  uint32_t startTime;
  uint32_t duration;
  uint32_t distance;
  uint32_t steps;
  uint8_t remainingRecords;
};

typedef std::variant<Info, Params, Record> Message;

Message parseMessage(const QByteArray& m);
}