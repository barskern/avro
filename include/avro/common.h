/******************************************************************************
 * File:             common.h
 *
 * Author:           Ole Martin Ruud
 * Created:          01/20/21
 * Description:      Common helpers and functions.
 *****************************************************************************/

#ifndef AVRO_COMMON_H
#define AVRO_COMMON_H

// Bit Operation macros
#define sbi(b, n) ((b) |= (1 << (n)))    // Set bit number n in byte b
#define cbi(b, n) ((b) &= (~(1 << (n)))) // Clear bit number n in byte b
#define fbi(b, n) ((b) ^= (1 << (n)))    // Flip bit number n in byte b
#define rbi(b, n) ((b) & (1 << (n)))     // Read bit number n in byte b

#ifndef AVRO_DISABLE_DEBUG
#include <avr/io.h>

// Port used for showing internal values
#define DEBUG_PORT PORTK
#define DEBUG_DDR DDRK

void init_debug() { DEBUG_DDR = 0xff; }
void write_debug(uint8_t b) { DEBUG_PORT = b; }
#else
void init_debug() {}
void write_debug(uint8_t b) {}
#endif

#endif /* ifndef AVRO_COMMON_H */
