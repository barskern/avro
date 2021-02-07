/******************************************************************************
 * File:             rotary.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/07/21
 * Description:      A simple interface to the rotary encoder using interrupts.
 *                   NB! Uses INT2 as external interrupt handler.
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
#define ROTARY_PINA PD2
#define ROTARY_PINB PD3
#endif

volatile int8_t offset;

void init_rotary() {
  // Ensure rotary pins are inputs
  ROTARY_DDR &= ~(1 << ROTARY_PINA) & ~(1 << ROTARY_PINB);

  // Enable any edge interrupt for int2
  EICRA |= (1 << ISC20);
  EIMSK |= (1 << INT2);
}

int8_t rotary_read() {
  int8_t tmp = offset;
  offset = 0;
  return tmp;
}

ISR(INT2_vect) {
  // TODO how do I debouce this more effectivly? Sometimes it skips a turn.
  // See https://www.best-microcontroller-projects.com/rotary-encoder.html
  bool a = (ROTARY_PIN & (1 << ROTARY_PINA)) != 0;
  bool b = (ROTARY_PIN & (1 << ROTARY_PINB)) != 0;

  if (a != b) {
    // CW
    if (offset < INT8_MAX)
      offset++;
  } else {
    // CCW
    if (offset > INT8_MIN)
      offset--;
  }
}

#endif /* ifndef AVRO_ROTARY_H */
