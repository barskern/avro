/******************************************************************************
 * File:             lcd.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/07/21
 * Description:      Communicate with LCD across TWI interface. Based on
 *                   source code provided Richard Antony and Steven Bos in
 *                   CS4120.
 *****************************************************************************/

#ifndef AVRO_LCD_H
#define AVRO_LCD_H

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

#include <avro/twi.h>

#define LCD_WIDTH 16

// PUBLIC

void init_lcd();

void lcd_clear();
void lcd_send_command(uint8_t value);
void lcd_write_char(char value);
void lcd_write(char *value);
void lcd_set_cursor(uint8_t col, uint8_t row);

// PRIVATE

// The default TWI address pre-programmed into the PCF8574T on the LCD module
#define LCD_TWI_ADDRESS 0x27

// LCD command definitions
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT_LEFT 0x10
#define LCD_CURSORSHIFT_RIGHT 0x14
#define LCD_FUNCTIONSET 0x20 // Need to set to 4-bit mode
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// Flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// Flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// Flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// Flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00 // Need to set to 4-bit mode
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// Flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

// Flags for mode when sending
#define LCD_COMMAND_MODE 0x00
#define LCD_DATA_MODE 0x01

#define LCD_RS_BIT 0b00000001     // Register select bit
#define LCD_RW_BIT 0b00000010     // Read/Write bit
#define LCD_ENABLE_BIT 0b00000100 // Enable bit

void _send_nibble(uint8_t value);
void _send_byte(uint8_t value, uint8_t mode);

void init_lcd() {
  _delay_ms(50);

  // Try to put  into 4 bit mode 3 times for good measure
  _send_nibble(0x03 << 4);
  _delay_us(4500);
  _send_nibble(0x03 << 4);
  _delay_us(4500);
  _send_nibble(0x03 << 4);
  _delay_us(150);

  // Finally, set to 4-bit interface
  _send_nibble(0x02 << 4);

  // set # lines, font size, etc.
  // Set 4-bit interface mode.
  lcd_send_command(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);

  // Turn the display on with cursor and blinking by default
  lcd_send_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON |
                   LCD_BLINKON);

  lcd_send_command(LCD_CLEARDISPLAY);
  _delay_us(2000);

  // Initialize to default text direction (for roman languages)
  lcd_send_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

  lcd_send_command(LCD_RETURNHOME);
  _delay_us(2000); // this command takes a long time!
}

void lcd_clear() { _send_byte(LCD_CLEARDISPLAY, LCD_COMMAND_MODE); }

void lcd_send_command(uint8_t value) { _send_byte(value, LCD_COMMAND_MODE); }

void lcd_write_char(char value) { _send_byte(value, LCD_DATA_MODE); }

void lcd_write(char *str) {
  uint8_t i = 0;
  while (str[i] != '\0') {
    _send_byte(str[i++], LCD_DATA_MODE);
  }
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
  uint8_t offset = row == 0 ? 0x00 : 0x40;
  lcd_send_command(LCD_SETDDRAMADDR | (col + offset));
}

void _send_byte(uint8_t value, uint8_t mode) {
  uint8_t high_nibble = value & 0xf0;
  uint8_t low_nibble = (value << 4) & 0xf0;

  _send_nibble(high_nibble | mode);
  _send_nibble(low_nibble | mode);
}

void _send_nibble(uint8_t value) {
  twi_send_blocking(LCD_TWI_ADDRESS, (value | LCD_BACKLIGHT) | LCD_ENABLE_BIT);
  _delay_us(1); // enable pulse must be >450ns

  twi_send_blocking(LCD_TWI_ADDRESS, (value | LCD_BACKLIGHT) & ~LCD_ENABLE_BIT);
  _delay_us(50); // commands need > 37us to settle
}

#endif /* ifndef AVRO_LCD_H */
