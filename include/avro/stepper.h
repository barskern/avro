/******************************************************************************
 * File:             stepper.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/07/21
 * Description:      A simple interface to control a stepper motor. NB! Uses
 *                   TIMER4 when sending move commands.
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
void stepper_step_cw();
void stepper_step_ccw();

void stepper_move(int32_t steps);
bool stepper_done();
void stepper_stop();

// PRIVATE

const uint8_t _stepper_states[] = {
    0b0011,
    0b0110,
    0b1100,
    0b1001,
};

volatile int32_t _stepper_offset = 0;
uint8_t _stepper_index = 0;

void init_stepper() {
  STEPPER_DDR |= 0x0f;
  STEPPER_PORT = _stepper_states[_stepper_index];

  // Setup interval (2 ms)
  OCR4A = F_CPU / 500;

  // Set CTC mode
  TCCR4B |= (1 << WGM42);

  // Enable compare A interrupt
  TIMSK4 |= (1 << OCIE4A);
}

void stepper_stop() { _stepper_offset = 0; }

void stepper_move(int32_t steps) {
  _stepper_offset += steps;

  // Start counter with clock (no prescaler)
  TCCR4B |= (1 << CS40);
}

void stepper_step_ccw() {
  _stepper_index = (_stepper_index + 1) % sizeof(_stepper_states);
  STEPPER_PORT = _stepper_states[_stepper_index];
}

void stepper_step_cw() {
  _stepper_index = (_stepper_index - 1) % sizeof(_stepper_states);
  STEPPER_PORT = _stepper_states[_stepper_index];
}

bool stepper_done() { return _stepper_offset == 0; }

ISR(TIMER4_COMPA_vect) {
  if (_stepper_offset < 0) {
    stepper_step_ccw();
    _stepper_offset++;
  } else if (_stepper_offset > 0) {
    stepper_step_cw();
    _stepper_offset--;
  } else {
    TCCR4B &= ~(1 << CS40);
  }
}

#endif /* ifndef AVRO_STEPPER_H */
