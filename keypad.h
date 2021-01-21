/******************************************************************************
 * File:             keypad.h
 *
 * Author:           Ole Martin Ruud
 * Created:          01/20/21
 * Description:      Functions to interact with 4x4 keypad.
 *****************************************************************************/

#include <avr/io.h>

#define KEYPAD_DDR DDRK
#define KEYPAD_PORT PORTK
#define KEYPAD_PIN PINK

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

// clang-format off
char keypad_encodings[] = {
    '1', '4', '7', '*',
    '2', '5', '8', '0',
    '3', '6', '9', '#',
    'a', 'b', 'c', 'd',
};
// clang-format on

char get_first_symbol(uint16_t mask) {
  for (uint8_t i = 0; i < 16; ++i) {
    if ((mask & (1 << i)) != 0) {
      return keypad_encodings[i];
    }
  }
  return ' ';
}
