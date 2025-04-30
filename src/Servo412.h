#ifndef SERVO412_H
#define SERVO412_H

#include <Arduino.h>

class Servo {
public:
  Servo();
  uint8_t attach(uint8_t pin, uint16_t min = 544, uint16_t max = 2400);
  void detach();
  void write(uint8_t angle);

private:
  uint8_t servoPin;
  int8_t minPulse, maxPulse;
  uint16_t ticks;
  bool active;
};

#endif
