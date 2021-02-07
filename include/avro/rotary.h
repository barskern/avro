/******************************************************************************
 * File:             rotary.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/07/21
 * Description:      A simple interface to the rotary encoder using interrupts.
 *                   NB! Uses INT2 and INT4.
 *****************************************************************************/

#ifndef AVRO_ROTARY_H
#define AVRO_ROTARY_H

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>

#ifndef ROTARY_CUSTOM_PORT
#define ROTARY_DDR DDRD
#define ROTARY_PORT PORTD
#define ROTARY_PIN PIND

// NB! Must be pin with INT2 interrupt
#define ROTARY_PIN_A PD2
#define ROTARY_PIN_B PD3

#define ROTARY2_DDR DDRE
#define ROTARY2_PORT PORTE
#define ROTARY2_PIN PINE

// NB! Must be pin with INT4 interrupt
#define ROTARY2_PIN_BTN PE4
#endif

// PUBLIC

void init_rotary();
int8_t rotary_read_offset();
bool rotary_read_pressed();

// PRIVATE

volatile int8_t _offset;
volatile bool _pressed;

void init_rotary() {
  // Ensure rotary pins are inputs
  ROTARY_DDR &= ~(1 << ROTARY_PIN_A) & ~(1 << ROTARY_PIN_B);
  ROTARY2_DDR &= ~(1 << ROTARY2_PIN_BTN);

  // Enable any edge interrupt for int2
  EICRA |= (1 << ISC20);

  // Enable falling edge interrupt for int4
  EICRB |= (1 << ISC41);

  // Enable both INT2 and INT4
  EIMSK |= (1 << INT2) | (1 << INT4);
}

int8_t rotary_read_offset() {
  int8_t tmp = _offset;
  _offset = 0;
  return tmp;
}

bool rotary_read_pressed() {
  bool tmp = _pressed;
  _pressed = false;
  return tmp;
}

ISR(INT4_vect) { _pressed = true; }

ISR(INT2_vect) {
  // TODO debounce this more effectivly with a simple state machine.
  // See https://www.best-microcontroller-projects.com/rotary-encoder.html
  bool a = (ROTARY_PIN & (1 << ROTARY_PIN_A)) != 0;
  bool b = (ROTARY_PIN & (1 << ROTARY_PIN_B)) != 0;

  if (a != b) {
    // CW
    if (_offset < INT8_MAX)
      _offset++;
  } else {
    // CCW
    if (_offset > INT8_MIN)
      _offset--;
  }
}

#endif /* ifndef AVRO_ROTARY_H */
