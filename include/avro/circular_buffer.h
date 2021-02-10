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
#include <stdbool.h>
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
uint8_t circular_buffer_len(circular_buffer_t *buf);
void circular_buffer_advance(uint8_t amount, circular_buffer_t *buf);
uint8_t circular_buffer_read(uint8_t *dest, uint8_t max_len,
                             circular_buffer_t *buf);
uint8_t circular_buffer_read_and_advance(uint8_t *dest, uint8_t max_len,
                                         circular_buffer_t *buf);

// PRIVATE

uint8_t _circular_buffer_read(uint8_t *dest, uint8_t max_len,
                              circular_buffer_t *buf, bool advance);

uint8_t circular_buffer_len(circular_buffer_t *buf) {
  uint8_t bstart = buf->start;
  uint8_t bend = buf->end;

  if (bstart <= bend) {
    return bend - bstart;
  } else {
    return (buf->len - bstart) + bend;
  }
}

circular_buffer_status_t
circular_buffer_write(circular_buffer_t *buf, const uint8_t *src, uint8_t len) {
  uint8_t bstart = buf->start;
  uint8_t bend = buf->end;

  if (bstart <= bend) {
    // Buffer only has continuos data, so we can insert after end and possibly
    // before start.
    uint8_t avaliable_continous_back = buf->len - bend;
    uint8_t avaliable_continous_front = bstart;
    if (avaliable_continous_back + avaliable_continous_front < len) {
      // The data cannot fit, so we insert none of it
      return CIRCULAR_BUFFER_FULL;
    }

    if (len <= avaliable_continous_back) {
      // The data can fit continuosly at the end of the buffer
      memcpy(buf->buf + bend, src, len * sizeof(uint8_t));
      bend += len;

      assert(bend <= buf->len);
    } else {
      // We have space, but we have to split the data across the buffer boundary
      assert(avaliable_continous_back < len);

      memcpy(buf->buf + bend, src, avaliable_continous_back * sizeof(uint8_t));
      bend += avaliable_continous_back;
      assert(bend == buf->len);

      memcpy(buf->buf, src + avaliable_continous_back,
             (len - avaliable_continous_back) * sizeof(uint8_t));
      bend = len - avaliable_continous_back;
      assert(bend <= bstart);
    }
  } else {
    // Buffer already has non-continuos data, so we have to insert data between
    // bend and bstart.
    uint8_t avaliable_continous = bstart - bend;
    if (avaliable_continous < len) {
      // The data cannot fit, so we insert none of it
      return CIRCULAR_BUFFER_FULL;
    }

    // The data can fit continuosly between bend and bstart
    memcpy(buf->buf + bend, src, len * sizeof(uint8_t));
    bend += len;
    assert(bend <= bstart);
  }

  // NB! We have to ensure to update the actual end of the buffer.
  buf->end = bend;

  return CIRCULAR_BUFFER_OK;
}

void circular_buffer_advance(uint8_t amount, circular_buffer_t *buf) {
  // By reading these values first, we can safely interrupt this function and
  // append to the end without it "breaking" the datastructure. This is
  // because buf->end is ONLY increased.
  uint8_t bstart = buf->start;
  uint8_t bend = buf->end;

  // Saturate amount to max size of buffer, so if amount is greater than
  // amount of data in buffer, we simply "eat" all data, leaving the buffer
  // empty.
  if (bstart <= bend) {
    amount = amount <= bend - bstart ? amount : bend - bstart;
    buf->start = bstart + amount;

  } else {
    amount =
        amount <= buf->len - bstart + bend ? amount : buf->len - bstart + bend;

    buf->start = (bstart + amount) % buf->len;
  }
}

uint8_t _circular_buffer_read(uint8_t *dest, uint8_t max_len,
                              circular_buffer_t *buf, bool advance) {

  // By reading these values first, we can safely interrupt this function and
  // append to the end without it "breaking" the datastructure. This is
  // because writing ONLY changes buf->end, and reading only changes
  // buf->start.
  uint8_t bstart = buf->start;
  uint8_t bend = buf->end;

  if (bstart == bend) {
    return 0;
  } else if (bstart < bend) {
    // Buffer has only continuos data, so reading is simply between bstart
    // and bend, and then move bstart by the amount read.

    uint8_t read_len = bend - bstart < max_len ? bend - bstart : max_len;
    memcpy(dest, buf->buf + bstart, read_len);

    if (advance) {
      bstart += read_len;
      assert(bstart <= bend);

      // NB! We have to remember to update the actual buffer start before
      // exiting. This is an "atomic" operation and will enable the write
      // routine to write to more of the buffer.
      buf->start = bstart;
    }

    return read_len;
  } else {
    // Buffer has non-continuos data, so we first have to read from the
    // bstart to buf->len, and then from buf-buf to bend.

    uint8_t read_len_back =
        buf->len - bstart < max_len ? buf->len - bstart : max_len;
    memcpy(dest, buf->buf + bstart, read_len_back);

    uint8_t read_len_front =
        bend < max_len - read_len_back ? bend : max_len - read_len_back;
    memcpy(dest + read_len_back, buf->buf, read_len_front);

    if (advance) {
      // Use modulo to make bstart zero if we read to the end of the buffer.
      bstart = (bstart + read_len_back + read_len_front) % buf->len;

      // NB! We have to remember to update the actual buffer start before
      // exiting. This is an "atomic" operation and will enable the write
      // routine to write to more of the buffer.
      buf->start = bstart;
    }

    return read_len_back + read_len_front;
  }
}

uint8_t circular_buffer_read(uint8_t *dest, uint8_t max_len,
                             circular_buffer_t *buf) {
  return _circular_buffer_read(dest, max_len, buf, false);
}

uint8_t circular_buffer_read_and_advance(uint8_t *dest, uint8_t max_len,
                                         circular_buffer_t *buf) {
  return _circular_buffer_read(dest, max_len, buf, true);
}

#endif /* ifndef AVRO_CIRCULAR_BUFFER_H */
