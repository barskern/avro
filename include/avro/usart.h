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

#include <avr/sleep.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>

#include "circular_buffer.h"

#define TX_BUFFER_SIZE 32
#define RX_BUFFER_SIZE 32

// PUBLIC

void init_usart();

void usart_send_byte(uint8_t byte);
void usart_send_bytes(const uint8_t *buf, uint8_t len);
void usart_send_string(const char *string);

void usart_send_byte_blocking(uint8_t byte);
void usart_send_bytes_blocking(const uint8_t *buf, uint8_t len);
void usart_send_string_blocking(const char *string);

void usart_recv_drop_until_blocking(const char *needle, uint8_t *recv_buf,
                                    uint8_t len);
uint8_t usart_recv_take_until_blocking(const char *needle, uint8_t *recv_buf,
                                       uint8_t len);

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

void usart_recv_drop_until_blocking(const char *needle, uint8_t *recv_buf,
                                    uint8_t len) {
  uint8_t needle_len = strlen(needle);
  uint8_t *needle_start;
  uint8_t total_recv_bytes = 0;

  while (1) {
    // We are always dropping prior data, so we can advance while we read.
    uint8_t recv_bytes =
        circular_buffer_read(recv_buf + total_recv_bytes,
                             len - 1 - total_recv_bytes, &_usart_recv_buffer);
    total_recv_bytes += recv_bytes;

    recv_buf[total_recv_bytes] = '\0';
    needle_start = (uint8_t *)strstr((char *)recv_buf, needle);
    if (needle_start) {
      // Only advance the difference between the previous position and the
      // start of the needle, this keeps everything after the needle in the
      // recv buffer.
      circular_buffer_advance((needle_start - recv_buf) -
                                  (total_recv_bytes - recv_bytes),
                              &_usart_recv_buffer);
      return;
    } else {
      circular_buffer_advance(recv_bytes, &_usart_recv_buffer);
    }

    // If we filled the buffer, move the last values to the front, and start
    // reading to the middle of the buffer. This ensures that a calibrate
    // command is not dropped if it arrvives in the middle of a buffer move.
    if (total_recv_bytes >= len - 1) {
      total_recv_bytes = needle_len;
      memmove(recv_buf, recv_buf + len - 1 - total_recv_bytes,
              total_recv_bytes);
    }

    // Only sleep if we recveived zero bytes, as then we have to wait for the
    // next RX interrupt.
    if (recv_bytes == 0)
      sleep_mode();
  }
}

uint8_t usart_recv_take_until_blocking(const char *needle, uint8_t *recv_buf,
                                       uint8_t len) {
  uint8_t needle_len = strlen(needle);
  uint8_t *needle_start;
  uint8_t total_recv_bytes = 0;

  while (1) {
    // We want to keep data in recv buffer that is after what we take to
    // simplify interface.
    uint8_t recv_bytes =
        circular_buffer_read(recv_buf + total_recv_bytes,
                             len - 1 - total_recv_bytes, &_usart_recv_buffer);
    total_recv_bytes += recv_bytes;

    recv_buf[total_recv_bytes] = '\0';
    needle_start = (uint8_t *)strstr((char *)recv_buf, needle);
    if (needle_start) {
      // We found what we were searching for, so advance recv buffer to
      // the end of the needle to consume the data, then return the amount of
      // bytes read (excluding the needle).

      // Only advance the difference between the previous position and the
      // end of the needle, this keeps everything after the needle in the
      // recv buffer.
      circular_buffer_advance((needle_start - recv_buf) -
                                  (total_recv_bytes - recv_bytes) + needle_len,
                              &_usart_recv_buffer);
      return needle_start - recv_buf;
    } else {
      circular_buffer_advance(recv_bytes, &_usart_recv_buffer);
    }

    // If we filled the buffer, sooo this is an error and there isn't much to
    // do. Simply return what we have read so far.
    // TODO should be handled more gracefully.
    if (total_recv_bytes >= len - 1) {
      return total_recv_bytes;
    }

    // Only sleep if we recveived zero bytes, as then we have to wait for the
    // next RX interrupt.
    if (recv_bytes == 0)
      sleep_mode();
  }
}

void usart_send_byte(uint8_t byte) {
  if (_usart_is_sending) {
    circular_buffer_status_t status =
        circular_buffer_write(&_usart_send_buffer, &byte, 1);
    if (status != CIRCULAR_BUFFER_OK) {
      // TODO handle error!
    }
    // Say we are sending and ensure empty data registry interrupt is set
    _usart_is_sending = true;
    UCSR0B |= (1 << UDRIE0);
  } else {
    // Should not block due to _usart_is_sending is false (which means a
    // interrupt the the buffer is empty has already come)
    usart_send_byte_blocking(byte);
  }
}
void usart_send_bytes(const uint8_t *buf, uint8_t len) {
  // We simply append the data to the send buffer, and ensure that we are
  // sending when we are ready.
  circular_buffer_status_t status =
      circular_buffer_write(&_usart_send_buffer, buf, len);
  if (status != CIRCULAR_BUFFER_OK) {
    // TODO handle error!
  }

  // Enable empty data registry interrupt to start sending, and say that we
  // are currently sending.
  _usart_is_sending = true;
  UCSR0B |= (1 << UDRIE0);
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
  uint8_t data;
  uint8_t n = circular_buffer_read_and_advance(&data, 1, &_usart_send_buffer);
  if (n == 1) {
    UDR0 = data;
    _usart_is_sending = true;
    UCSR0B |= (1 << UDRIE0);
  } else {
    // Disable interrupt and say that we are not sending
    _usart_is_sending = false;
    UCSR0B &= ~(1 << UDRIE0);
  }
}

#endif /* ifndef AVRO_USART_H */
