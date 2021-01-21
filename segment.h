/******************************************************************************
 * File:             segment.h
 *
 * Author:           Ole Martin Ruud
 * Created:          01/20/21
 * Description:      Functions to interact with multi-segment display.
 *****************************************************************************/

// NB! Uses TIMER5 as internal interrupts to draw to display.

#include <avr/interrupt.h>
#include <avr/io.h>

#include <util/atomic.h>
#include <util/delay.h>

#define SEGMENT_DDR DDRA
#define SEGMENT_PORT PORTA
#define SEGMENT_PIN PINA

// uses only lower 4 bits of this port
#define SEGMENT_DIGIT_DDR DDRC
#define SEGMENT_DIGIT_PORT PORTC
#define SEGMENT_DIGIT_PIN PINC

#define REFRESH_RATE 50
#define DATA_LEN 4

// 0b10000000 = top
// 0b01000000 = right top
// 0b00100000 = right bottom
// 0b00010000 = bottom
// 0b00001000 = left bottom
// 0b00000100 = left top
// 0b00000010 = middle
// 0b00000001 = dot

const uint8_t SEGMENT_ENCODINGS_NUMBERS[] = {
    0b11111100, // zero
    0b01100000, // one
    0b11011010, // two
    0b11110010, // three
    0b01100110, // four
    0b10110110, // five
    0b10111110, // six
    0b11100000, // seven
    0b11111110, // eight
    0b11100110, // nine
};

const uint8_t SEGMENT_ENCODINGS_LETTERS[] = {
    0b11101110, // a
    0b00111110, // b
    0b10011100, // c
    0b01111010, // d
};

// Use two backing buffers to enable atomic pointer swaps as a synchronization
// measure. This means that the read-pointer (rdata) is readonly by anyone and
// hence CANNOT be written too. Further the main execution owns the
// write-pointer (wdata) and can write to it, BUT it should never be read in an
// interrupt.
char _left[DATA_LEN] = {0};
char _right[DATA_LEN] = {0};

volatile char *_wdata = _left;
volatile char *_rdata = _right;

void _swap_data_ptrs() {
  // Swap the pointers atomically so that the read-pointer points to the
  // newly filled data, while the write-pointer will point to the outdated
  // (old) data.
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    volatile char *tmp = _wdata;
    _wdata = _rdata;
    _rdata = tmp;
  }

  // After the previous swap, wdata now contains the old data. As anyone can
  // read from the read-pointer, we copy over every character so that wdata has
  // the latest changes from the previous write cycle.
  for (uint8_t i = 0; i < DATA_LEN; ++i) {
    _wdata[i] = _rdata[i];
  }
}

void init_segment() {
  SEGMENT_DDR = 0xff;
  SEGMENT_PORT = 0x00;

  SEGMENT_DIGIT_DDR = 0x0f;
  SEGMENT_DIGIT_PORT = 0x0f;

  // Setup interval
  OCR5B = 0x0000;
  OCR5C = F_CPU / (2 * DATA_LEN * REFRESH_RATE);
  OCR5A = F_CPU / (DATA_LEN * REFRESH_RATE);

  TCCR5B |=
      // Select clock (no prescaling)
      (1 << CS50) |
      // Set CTC mode
      (1 << WGM52);

  // Enable compare B and C interrupt
  TIMSK5 |= (1 << OCIE5B) | (1 << OCIE5C);
}

void segment_clear() {
  // Set all data to empty
  for (uint8_t i = 0; i < DATA_LEN; ++i)
    _wdata[i] = 0;
  _swap_data_ptrs();
}

void segment_write_char(char c) {
  // Shift data to the left and add new symbol at the end
  for (uint8_t i = 0; i < DATA_LEN - 1; ++i)
    _wdata[i] = _wdata[i + 1];
  _wdata[DATA_LEN - 1] = c;
  _swap_data_ptrs();
}

void _show_single_digit(uint8_t d) {
  if (0 <= d && d <= 9)
    SEGMENT_PORT = SEGMENT_ENCODINGS_NUMBERS[d];
  else
    SEGMENT_PORT = 0;
}

void _enable_digit(uint8_t n) { SEGMENT_DIGIT_PORT &= ~(1 << n); }

void _disable_digit(uint8_t n) { SEGMENT_DIGIT_PORT |= (1 << n); }

void _show_char(char c) {
  if ('a' <= c && c <= 'd')
    SEGMENT_PORT = SEGMENT_ENCODINGS_LETTERS[c - 'a'];
  else if ('0' <= c && c <= '9')
    SEGMENT_PORT = SEGMENT_ENCODINGS_NUMBERS[c - '0'];
  else
    SEGMENT_PORT = 0;
}

void _show_data(char data[4]) {
  for (int i = 3; i >= 0; --i) {
    _show_char(data[3 - i]);
    SEGMENT_DIGIT_PORT &= ~(1 << i);
    _delay_us(1);
    SEGMENT_DIGIT_PORT |= (1 << i);
    SEGMENT_PORT = 0;
  }
}

volatile uint8_t _current_digit = 0;

ISR(TIMER5_COMPB_vect) {
  _enable_digit(_current_digit);
  _show_char(_rdata[DATA_LEN - 1 - _current_digit]);
}

ISR(TIMER5_COMPC_vect) {
  _disable_digit(_current_digit);
  _show_char(' ');

  if (0 <= _current_digit && _current_digit < DATA_LEN - 1)
    _current_digit++;
  else
    _current_digit = 0;
}
