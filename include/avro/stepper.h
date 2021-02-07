/******************************************************************************
 * File:             stepper.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/07/21
 * Description:      A simple interface to control a stepper motor.
 *****************************************************************************/

#ifndef AVRO_STEPPER_H
#define AVRO_STEPPER_H

#include <stdbool.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#ifndef STEPPER_CUSTOM_PORT
#define STEPPER_DDR DDRC
#define STEPPER_PORT PORTC
#define STEPPER_PIN PINC
#endif

// PUBLIC

void init_stepper();
void stepper_cw();
void stepper_ccw();

void stepper_move(int32_t steps);

// PRIVATE

const uint8_t _states[] = {
    0b0011,
    0b0110,
    0b1100,
    0b1001,
};

volatile int32_t _offset = 0;
uint8_t _index = 0;

void init_stepper() {
  STEPPER_DDR |= 0x0f;
  STEPPER_PORT = _states[_index];

  // Setup interval (2 ms)
  OCR4A = F_CPU / 500;

  // Set CTC mode
  TCCR4B |= (1 << WGM42);

  // Enable compare A interrupt
  TIMSK4 |= (1 << OCIE4A);
}

void stepper_move(int32_t steps) {
  _offset = steps;

  // Start counter with clock (no prescaler)
  TCCR4B |= (1 << CS40);
}

void stepper_ccw() {
  _index = (_index + 1) % sizeof(_states);
  STEPPER_PORT = _states[_index];
}

void stepper_cw() {
  _index = (_index - 1) % sizeof(_states);
  STEPPER_PORT = _states[_index];
}

ISR(TIMER4_COMPA_vect) {
  if (_offset < 0) {
    stepper_ccw();
    _offset++;
  } else if (_offset > 0) {
    stepper_cw();
    _offset--;
  } else {
    TCCR4B &= ~(1 << CS40);
  }
}

#endif /* ifndef AVRO_STEPPER_H */
