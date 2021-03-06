/******************************************************************************
 * File:             keypad.h
 *
 * Author:           Ole Martin Ruud
 * Created:          01/20/21
 * Description:      Functions to interact with 4x4 keypad.
 *****************************************************************************/

#ifndef AVRO_KEYPAD_H
#define AVRO_KEYPAD_H

#include <avr/io.h>

#ifndef KEYPAD_CUSTOM_PORT
#define KEYPAD_DDR DDRK
#define KEYPAD_PORT PORTK
#define KEYPAD_PIN PINK
#endif

// PUBLIC

void init_keypad();
uint16_t read_keypad();
char get_first_symbol(uint16_t mask);

// PRIVATE

// clang-format off
const char KEYPAD_ENCODINGS[] = {
    '1', '4', '7', '*',
    '2', '5', '8', '0',
    '3', '6', '9', '#',
    'a', 'b', 'c', 'd',
};
// clang-format on

void init_keypad() {
  KEYPAD_DDR = 0xf0;
  KEYPAD_PORT = 0xff;
}

uint16_t read_keypad() {
  uint16_t mask = 0;

  for (uint8_t row = 0; row < 4; ++row) {
    KEYPAD_PORT &= ~(1 << (row + 4));

    // Only read last 4 bits (inputs), invert (as they are active low) and
    // finally add to mask with an offset.
    mask |= ((~KEYPAD_PIN & 0x0f) << (4 * row));

    KEYPAD_PORT |= (1 << (row + 4));
  }

  return mask;
}


char get_first_symbol(uint16_t mask) {
  for (uint8_t i = 0; i < 16; ++i) {
    if ((mask & (1 << i)) != 0) {
      return KEYPAD_ENCODINGS[i];
    }
  }
  return ' ';
}

#endif /* ifndef AVRO_KEYPAD_H */
