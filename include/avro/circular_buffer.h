/******************************************************************************
 * File:             circular_buffer.h
 *
 * Author:           Ole Martin Ruud
 * Created:          02/09/21
 * Description:      An implementation of a circular buffer, useful as a buffer
 *                   for sending and reciving data.
 *****************************************************************************/

#ifndef AVRO_CIRCULAR_BUFFER_H
#define AVRO_CIRCULAR_BUFFER_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

// PUBLIC

// A circular buffer which has a backing storage (buf) with a certain length
// (len), and the data that has been written to it can be found between the
// start and end, possibly passing the boundary of the backing storage.
typedef struct {
  uint8_t start;
  uint8_t end;
  uint8_t *buf;
  uint8_t len;
} circular_buffer_t;

typedef enum {
  CIRCULAR_BUFFER_OK,
  // Full means that the provided data cannot fit, and nothing has been written
  // into the buffer.
  CIRCULAR_BUFFER_FULL,
} circular_buffer_status_t;

circular_buffer_status_t circular_buffer_write(circular_buffer_t *buf,
                                               const uint8_t *src, uint8_t len);
uint8_t circular_buffer_read(uint8_t *dest, uint8_t max_len,
                             circular_buffer_t *buf);
uint8_t circular_buffer_len(circular_buffer_t *buf);

// PRIVATE

uint8_t circular_buffer_len(circular_buffer_t *buf) {
  if (buf->start <= buf->end) {
    return buf->end - buf->start;
  } else {
    return (buf->len - buf->start) + buf->end;
  }
}

circular_buffer_status_t
circular_buffer_write(circular_buffer_t *buf, const uint8_t *src, uint8_t len) {
  if (buf->start <= buf->end) {
    // Buffer only has continuos data, so we can insert after end and possibly
    // before start.
    uint8_t avaliable_continous_back = buf->len - buf->end;
    uint8_t avaliable_continous_front = buf->start;
    if (avaliable_continous_back + avaliable_continous_front < len) {
      // The data cannot fit, so we insert none of it
      return CIRCULAR_BUFFER_FULL;
    }

    if (len <= avaliable_continous_back) {
      // The data can fit continuosly at the end of the buffer
      memcpy(buf->buf + buf->end, src, len * sizeof(uint8_t));
      buf->end += len;

      assert(buf->end <= buf->len);
    } else {
      // We have space, but we have to split the data across the buffer boundary
      assert(avaliable_continous_back < len);

      memcpy(buf->buf + buf->end, src,
             avaliable_continous_back * sizeof(uint8_t));
      buf->end += avaliable_continous_back;
      assert(buf->end == buf->len);

      memcpy(buf->buf, src + avaliable_continous_back,
             (len - avaliable_continous_back) * sizeof(uint8_t));
      buf->end = len - avaliable_continous_back;
      assert(buf->end <= buf->start);
    }
  } else {
    // Buffer already has non-continuos data, so we have to insert data between
    // buf->end and buf->start.
    uint8_t avaliable_continous = buf->start - buf->end;
    if (avaliable_continous < len) {
      // The data cannot fit, so we insert none of it
      return CIRCULAR_BUFFER_FULL;
    }

    // The data can fit continuosly between buf->end and buf->start
    memcpy(buf->buf + buf->end, src, len * sizeof(uint8_t));
    buf->end += len;

    assert(buf->end <= buf->start);
  }

  return CIRCULAR_BUFFER_OK;
}

uint8_t circular_buffer_read(uint8_t *dest, uint8_t max_len,
                             circular_buffer_t *buf) {
  if (buf->start == buf->end) {
    return 0;
  } else if (buf->start < buf->end) {
    // Buffer has only continuos data, so reading is simply between buf->start
    // and buf->end, and then move buf->start by the amount read.

    uint8_t read_len =
        buf->end - buf->start < max_len ? buf->end - buf->start : max_len;
    memcpy(dest, buf->buf + buf->start, read_len);
    buf->start += read_len;

    return read_len;
  } else {
    // Buffer has non-continuos data, so we first have to read from the
    // buf->start to buf->len, and then from buf-buf to buf->end.

    uint8_t read_len_back =
        buf->len - buf->start < max_len ? buf->len - buf->start : max_len;

    memcpy(dest, buf->buf + buf->start, read_len_back);
    // Use modulo to make buf->start zero if we read to the end of the buffer.
    buf->start = (buf->start + read_len_back) % buf->len;

    uint8_t read_len_front =
        buf->end < max_len - read_len_back ? buf->end : max_len - read_len_back;
    memcpy(dest + read_len_back, buf->buf, read_len_front);
    buf->start = (buf->start + read_len_front) % buf->len;

    return read_len_back + read_len_front;
  }
}

#endif /* ifndef AVRO_CIRCULAR_BUFFER_H */
