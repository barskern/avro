/******************************************************************************
 * File:             usart.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/09/21
 * Description:      A simple interface to communicate with an external device
 *                   using USART.
 *****************************************************************************/

#ifndef AVRO_USART_H
#define AVRO_USART_H

#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdbool.h>
#include <string.h>
#include <util/delay.h>

#include "circular_buffer.h"

#define TX_BUFFER_SIZE 10
#define RX_BUFFER_SIZE 10

// PUBLIC

void init_usart();

void usart_send_byte(uint8_t byte);
void usart_send_bytes(const uint8_t *buf, uint8_t len);
void usart_send_string(const char *string);

void usart_send_byte_blocking(uint8_t byte);
void usart_send_bytes_blocking(const uint8_t *buf, uint8_t len);
void usart_send_string_blocking(const char *string);

uint8_t usart_recv_into(uint8_t *buf, uint8_t max_len);

// PRIVATE

volatile bool _usart_is_sending = false;

uint8_t _usart_send_buffer_inner[TX_BUFFER_SIZE];
volatile circular_buffer_t _usart_send_buffer = {
    .buf = _usart_send_buffer_inner,
    .len = sizeof(_usart_send_buffer_inner),
    .start = 0,
    .end = 0,
};

uint8_t _usart_recv_buffer_inner[RX_BUFFER_SIZE];
volatile circular_buffer_t _usart_recv_buffer = {
    .buf = _usart_recv_buffer_inner,
    .len = sizeof(_usart_recv_buffer_inner),
    .start = 0,
    .end = 0,
};

void init_usart() {
  // Set U2X (Double the USART Tx speed, to reduce clocking error)
  UCSR0A = (1 << U2X0);
  // RX Complete Int Enable, RX Enable, TX Enable
  UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
  // Asynchronous, No Parity, 1 stop, 8-bit data
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << UCPOL0);

  // 9600 baud, UBRR = 12, and  U2X must be set to '1' in UCSRA
  UBRR0H = 0;
  UBRR0L = 12;
}

void _clear_usart_interrupts() { UCSR0B &= ~(1 << UDRIE0) & ~(1 << RXCIE0); }
void _enable_usart_interrupts() { UCSR0B |= (1 << UDRIE0) | (1 << RXCIE0); }

uint8_t usart_recv_into(uint8_t *buf, uint8_t max_len) {
  _clear_usart_interrupts();
  uint8_t n = circular_buffer_read(buf, max_len, &_usart_recv_buffer);
  _enable_usart_interrupts();
  return n;
}

void usart_send_byte(uint8_t byte) {
  _clear_usart_interrupts();
  if (_usart_is_sending) {
    uint8_t tmp[] = {byte};
    circular_buffer_status_t status =
        circular_buffer_write(&_usart_send_buffer, tmp, sizeof(tmp));
    if (status != CIRCULAR_BUFFER_OK) {
      // TODO handle error!
    }
  } else {
    // Should not block due to _usart_is_sending is false (which means a
    // interrupt the the buffer is empty has already come)
    usart_send_byte_blocking(byte);
  }
  _enable_usart_interrupts();
}
void usart_send_bytes(const uint8_t *buf, uint8_t len) {
  _clear_usart_interrupts();
  if (_usart_is_sending) {
    // We are already sending, so we can just append to the send buffer, and
    // it will be sent automatically when the UDR0 is ready.
    //
    circular_buffer_status_t status =
        circular_buffer_write(&_usart_send_buffer, buf, len);
    if (status != CIRCULAR_BUFFER_OK) {
      // TODO handle error!
    }

  } else {
    // Write the data to the send buffer, and then start sending by activating
    // the interrupt on empty data registry.

    circular_buffer_status_t status =
        circular_buffer_write(&_usart_send_buffer, buf, len);
    if (status != CIRCULAR_BUFFER_OK) {
      // TODO handle error!
    }

    // Enable empty data registry interrupt to start sending, and say that we
    // are currently sending
    _usart_is_sending = true;
    UCSR0B |= (1 << UDRIE0);
  }
  _enable_usart_interrupts();
}

void usart_send_string(const char *str) {
  uint8_t len = strlen(str);
  usart_send_bytes((uint8_t *)str, len);
}

void usart_send_byte_blocking(uint8_t byte) {
  // Wait for asynchronous send to be finished
  while (_usart_is_sending)
    ;

  // Wait for Tx Buffer to become empty (check UDRE flag)
  while (!(UCSR0A & (1 << UDRE0)))
    ;
  UDR0 = byte;
}

void usart_send_bytes_blocking(const uint8_t *buf, uint8_t len) {
  for (uint8_t i = 0; i < len; ++i)
    usart_send_byte_blocking(buf[i]);
}

void usart_send_string_blocking(const char *str) {
  uint8_t idx = 0;
  while (str[idx] != '\0')
    usart_send_byte_blocking(str[idx++]);
}

ISR(USART0_RX_vect) {
  uint8_t val = UDR0;

  circular_buffer_status_t status =
      circular_buffer_write(&_usart_recv_buffer, &val, 1);
  if (status != CIRCULAR_BUFFER_OK) {
    // TODO handle error!
  }
}

ISR(USART0_UDRE_vect) {
  // Data registry empty and ready to send new byte
  uint8_t tmp[1];
  uint8_t n = circular_buffer_read(tmp, sizeof(tmp), &_usart_send_buffer);
  if (n > 0) {
    UDR0 = tmp[0];
    _usart_is_sending = true;
    UCSR0B |= (1 << UDRIE0);
  } else {
    // Disable interrupt and say that we are not sending
    _usart_is_sending = false;
    UCSR0B &= ~(1 << UDRIE0);
  }
}

#endif /* ifndef AVRO_USART_H */
