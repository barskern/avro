/******************************************************************************
 * File:             twi.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/07/21
 * Description:      A simple interface to communicate across TWI with both a
 *                   blocking and asynchronous interface.
 *****************************************************************************/

#ifndef AVRO_TWI_H
#define AVRO_TWI_H

#include <assert.h>
#include <stdbool.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include <util/twi.h>

// PUBLIC

void init_twi();

void twi_send_blocking(uint8_t addr, uint8_t value);
uint8_t twi_read_blocking(uint8_t addr);
void twi_transfer_blocking(uint8_t addr, uint8_t *buf, uint8_t len);

typedef enum {
  READY,
  PENDING,
  ERROR,
} twi_status_t;

twi_status_t twi_status();
void twi_send(uint8_t addr, uint8_t value);
void twi_read(uint8_t addr, uint8_t *value);
void twi_transfer(uint8_t addr, uint8_t *buf, uint8_t len);

// PRIVATE

void init_twi() {
  // Set SCL and SDA to input with pullup resistors
  DDRD &= ~(1 << PD0) & ~(1 << PD1);
  PORTD |= (1 << PD0) | (1 << PD1);

  // SCLf = 62.5KHz
  TWBR = 0;

  // Use prescaler 1
  TWSR = 0;

  // Enable TWI with ACK bit and interrupt
  TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
}

typedef enum {
  IDLE,
  BUSY_BLOCKING,
  SENT_START,
  SENT_READ_ADDR,
  SENT_READ_DATA,
  SENT_WRITE_ADDR,
  SENT_WRITE_DATA,
} twi_internal_state_t;

volatile twi_internal_state_t _twi_state = IDLE;
volatile uint8_t _twi_addr;
volatile uint8_t _twi_buf_index;
volatile uint8_t *_twi_buf;
volatile uint8_t _twi_buf_len;

twi_status_t twi_status() {
  switch (_twi_state) {
  case IDLE:
    return READY;
  case SENT_START:
  case SENT_READ_ADDR:
  case SENT_READ_DATA:
  case SENT_WRITE_ADDR:
  case SENT_WRITE_DATA:
  case BUSY_BLOCKING:
    return PENDING;
  default:
    return ERROR;
  }
}

void twi_send_blocking(uint8_t addr, uint8_t value) {
  uint8_t buf[] = {value};
  twi_transfer_blocking((addr << 1) | TW_WRITE, buf, sizeof(buf));
}

uint8_t twi_read_blocking(uint8_t addr) {
  uint8_t buf[1];
  twi_transfer_blocking((addr << 1) | TW_READ, buf, sizeof(buf));
  return buf[0];
}

void twi_transfer_blocking(uint8_t addr, uint8_t *buf, uint8_t len) {
  // Clear global interrupts while disabling TWI interrupts to ensure no funky
  // business happens.
  cli();
  while (_twi_state != IDLE)
    ;
  // Set into busy blocking state and disable interrupts for TWI
  // TWCR &= ~(1 << TWIE);
  _twi_state = BUSY_BLOCKING;
  sei();

  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
  while (!(TWCR & (1 << TWINT)))
    ;
  if (TW_STATUS != TW_START) {
    // TODO handle error
    goto cleanup;
  }

  TWDR = addr;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)))
    ;

  if (addr & TW_READ) {
    if (TW_STATUS != TW_MR_SLA_ACK) {
      // TODO handle error!
      goto cleanup;
    }

    for (int i = 0; i < len; ++i) {
      TWCR = (1 << TWINT) | (1 << TWEN);
      while (!(TWCR & (1 << TWINT)))
        ;

      if (TW_STATUS != TW_MR_DATA_ACK) {
        // TODO handle error!
        goto cleanup;
      }

      buf[i] = TWDR;
    }
  } else {
    if (TW_STATUS != TW_MT_SLA_ACK) {
      // TODO handle error!
      goto cleanup;
    }

    for (int i = 0; i < len; ++i) {
      TWDR = buf[i];
      TWCR = (1 << TWINT) | (1 << TWEN);

      while (!(TWCR & (1 << TWINT)))
        ;

      if (TW_STATUS != TW_MT_DATA_ACK) {
        // TODO handle error!
        goto cleanup;
      }
    }
  }
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

cleanup:
  _twi_state = IDLE;
  // TWCR |= (1 << TWIE);
}

void twi_send(uint8_t addr, uint8_t value) {
  uint8_t buf[] = {value};
  twi_transfer((addr << 1) | TW_WRITE, buf, sizeof(buf));
}

void twi_read(uint8_t addr, uint8_t *value) {
  twi_transfer((addr << 1) | TW_READ, value, 1);
}

void twi_transfer(uint8_t addr, uint8_t *buf, uint8_t len) {
  assert(len > 0);
  assert(_twi_state != BUSY_BLOCKING);

  _twi_buf_index = 0;
  _twi_buf = buf;
  _twi_buf_len = len;
  _twi_addr = addr;
  TWCR = (1 << TWINT) + (1 << TWEN) + (1 << TWSTA);
  _twi_state = SENT_START;
}

ISR(TWI_vect) {
  switch (_twi_state) {
  case SENT_START:
    if (TW_STATUS == TW_START) {
      TWDR = _twi_addr;
      TWCR = (1 << TWINT) | (1 << TWEN);
      _twi_state = _twi_addr & TW_READ ? SENT_READ_ADDR : SENT_WRITE_ADDR;
    } else {
      // TODO handle error!
    }
    break;
  case SENT_WRITE_ADDR:
    if (TW_STATUS == TW_MT_SLA_ACK) {
      TWDR = _twi_buf[_twi_buf_index++];
      TWCR = (1 << TWINT) | (1 << TWEN);
      _twi_state = SENT_WRITE_DATA;
    } else {
      // TODO handle error!
    }
    break;
  case SENT_WRITE_DATA:
    if (TW_STATUS == TW_MT_DATA_ACK) {
      if (_twi_buf_index < _twi_buf_len) {
        TWDR = _twi_buf[_twi_buf_index++];
        TWCR = (1 << TWINT) | (1 << TWEN);
        _twi_state = SENT_WRITE_DATA;
      } else {
        // Entire buffer was sent, send stop condition
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
        _twi_state = IDLE;
      }
    } else {
      // TODO handle error!
    }
    break;
  case SENT_READ_ADDR:
    if (TW_STATUS == TW_MR_SLA_ACK) {
      TWCR = (1 << TWINT) | (1 << TWEN);
      _twi_state = SENT_READ_DATA;
    } else {
      // TODO handle error!
    }
    break;
  case SENT_READ_DATA:
    if (TW_STATUS == TW_MR_DATA_ACK) {
      if (_twi_buf_index < _twi_buf_len) {
        _twi_buf[_twi_buf_index++] = TWDR;
        TWCR = (1 << TWINT) | (1 << TWEN);
        _twi_state = SENT_READ_DATA;
      } else {
        // Entire buffer was filled with data, send stop condition
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
        _twi_state = IDLE;
      }
    } else {
      // TODO handle error!
    }
    break;
  case BUSY_BLOCKING:
    // This shouldn't happend because interupts for TWI are disabled when
    // doing blocking send.
    break;
  default:
    break;
  }
}

#endif /* ifndef AVRO_TWI_H */
