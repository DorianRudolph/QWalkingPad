#pragma once
#include <QByteArray>

QByteArray messageByte(uint8_t k, uint8_t v);
QByteArray messageInt(uint8_t k, uint32_t v);
QByteArray query();
QByteArray queryParams();
QByteArray setSpeed(uint8_t speed);
QByteArray setStartSpeed(uint8_t speed);
QByteArray setMode(uint8_t mode);
QByteArray start();
QByteArray syncRecord(uint8_t n);