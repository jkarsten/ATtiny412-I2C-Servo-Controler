#if defined(ARDUINO_ARCH_MEGAAVR) && defined(__AVR_ATtiny412__)

#include <Arduino.h>
#include "Servo412.h"

#define DEFAULT_PULSE_WIDTH 1500
#define MIN_PULSE_WIDTH 544
#define MAX_PULSE_WIDTH 2400
#define REFRESH_INTERVAL 20000

#if (F_CPU > 10000000)
  #define usToTicks(_us)    ((((_us) / 2) * clockCyclesPerMicrosecond()))
  #define TRIM_DURATION     51
#else
  #define usToTicks(_us)    ((_us) * clockCyclesPerMicrosecond())
  #define TRIM_DURATION     102
#endif

typedef struct {
  struct {
    uint8_t port;
    uint8_t bitmask;
    bool isActive;
  } Pin;
  uint16_t ticks;
} servo_t;

static servo_t servo;
static uint32_t currentCycleTicks = 0;
static int8_t currentServoIndex = -1;
#define _timer TCB0

ISR(TCB0_INT_vect) {
  if (currentServoIndex < 0) {
    if (currentCycleTicks < usToTicks(REFRESH_INTERVAL)) {
      uint32_t tval = usToTicks(REFRESH_INTERVAL) - currentCycleTicks;
      _timer.CCMP = (tval > 65535) ? 65535 : tval;
      currentCycleTicks += _timer.CCMP;
      _timer.INTFLAGS = TCB_CAPT_bm;
      return;
    } else {
      currentCycleTicks = 0;
    }
  } else {
    if (servo.Pin.isActive) {
      ((PORT_t *)&PORTA + servo.Pin.port)->OUTCLR = servo.Pin.bitmask;
    }
  }

  currentServoIndex++;

  if (currentServoIndex == 0 && servo.Pin.isActive) {
    ((PORT_t *)&PORTA + servo.Pin.port)->OUTSET = servo.Pin.bitmask;
    _timer.CCMP = servo.ticks;
    currentCycleTicks += servo.ticks;
  } else {
    uint32_t tval = usToTicks(REFRESH_INTERVAL) - currentCycleTicks;
    _timer.CCMP = (tval > 65535) ? 65535 : tval;
    currentCycleTicks += _timer.CCMP;
    currentServoIndex = -1;
  }

  _timer.INTFLAGS = TCB_CAPT_bm;
}

static void initISR() {
#if (F_CPU > 10000000)
  _timer.CTRLA = TCB_CLKSEL_DIV2_gc;
#else
  _timer.CTRLA = TCB_CLKSEL_DIV1_gc;
#endif
  _timer.CTRLB = TCB_CNTMODE_INT_gc;
  _timer.CCMP = 0x8000;
  _timer.INTCTRL = TCB_CAPTEI_bm;
  _timer.CTRLA |= TCB_ENABLE_bm;
}

static void finISR() {
  _timer.INTCTRL = 0;
}

Servo::Servo()
  : servoPin(0), minPulse(0), maxPulse(0), ticks(usToTicks(DEFAULT_PULSE_WIDTH)), active(false) {}

uint8_t Servo::attach(uint8_t pin, uint16_t min, uint16_t max) {
  uint8_t bitmask = digitalPinToBitMask(pin);
  if (bitmask == NOT_A_PIN) return 255;

  this->servoPin = pin;
  this->minPulse = (int8_t)((MIN_PULSE_WIDTH - min) / 4);
  this->maxPulse = (int8_t)((MAX_PULSE_WIDTH - max) / 4);

  uint8_t port = digitalPinToPort(pin);
  ((PORT_t *)&PORTA + port)->DIRSET = bitmask;

  servo.Pin.port = port;
  servo.Pin.bitmask = bitmask;
  servo.Pin.isActive = true;
  servo.ticks = this->ticks;
  this->active = true;

  initISR();
  return 0;
}

void Servo::detach() {
  servo.Pin.isActive = false;
  this->active = false;
  finISR();
}

void Servo::write(uint8_t value) {
  if (!active) return;
  if (value > 180) value = 180;

  uint16_t us = map(value, 0, 180,
                    MIN_PULSE_WIDTH - minPulse * 4,
                    MAX_PULSE_WIDTH - maxPulse * 4);

  us = constrain(us,
                 (uint16_t)(MIN_PULSE_WIDTH - minPulse * 4),
                 (uint16_t)(MAX_PULSE_WIDTH - maxPulse * 4));

  ticks = usToTicks(us) - TRIM_DURATION;
  servo.ticks = ticks;
}

#endif
