#include <assert.h>
#include <stdio.h>

#include "../include/avro/circular_buffer.h"

#define TEST_START printf("Running: %s\n", __func__)
#define TEST_END printf("Success: %s\n", __func__)

void circular_buffer_is_writable_test();
void circular_buffer_is_readable_test();
void circular_buffer_push_across_boundary_test();
void circular_buffer_read_across_boundary_test();
void circular_buffer_advance_test();
void circular_buffer_advance_across_boundary_test();

#define TESTS 6
void (*tests[TESTS])() = {
    circular_buffer_is_writable_test,
    circular_buffer_is_readable_test,
    circular_buffer_push_across_boundary_test,
    circular_buffer_read_across_boundary_test,
    circular_buffer_advance_test,
    circular_buffer_advance_across_boundary_test,
};

int main(void) {
  puts("#################################");
  puts("Running tests for circular buffer");
  puts("#################################");

  for (int i = 0; i < TESTS; ++i) {
    (*tests[i])();
  }

  puts("#################################");
  puts("Tests successful!");
  puts("#################################");

  return 0;
}

void circular_buffer_is_writable_test() {
  TEST_START;

  uint8_t _buf[10];
  circular_buffer_t buf = {
      .buf = _buf,
      .len = sizeof(_buf),
      .start = 0,
      .end = 0,
  };

  uint8_t src[] = {0xff, 0xff, 0x11, 0x22};
  circular_buffer_write(&buf, src, sizeof(src));

  assert(circular_buffer_len(&buf) == 4);

  TEST_END;
}

void circular_buffer_is_readable_test() {
  TEST_START;

  uint8_t _buf[10] = {0xff, 0xff, 0x11, 0x22};
  circular_buffer_t buf = {
      .buf = _buf,
      .len = sizeof(_buf),
      .start = 0,
      .end = 4,
  };

  uint8_t dest[5];
  circular_buffer_read(dest, sizeof(dest), &buf);

  for (int i = 0; i < 4; ++i) {
    assert(dest[i] == _buf[i]);
  }

  TEST_END;
}

void circular_buffer_push_across_boundary_test() {
  TEST_START;

  uint8_t _buf[10];
  circular_buffer_t buf = {
      .buf = _buf,
      .len = sizeof(_buf),
      .start = 8,
      .end = 8,
  };

  uint8_t src[] = {0xff, 0xff, 0x11, 0x22};
  circular_buffer_write(&buf, src, sizeof(src));

  assert(circular_buffer_len(&buf) == 4);

  uint8_t dest[5];
  circular_buffer_read(dest, sizeof(dest), &buf);

  for (int i = 0; i < 4; ++i) {
    assert(dest[i] == src[i]);
  }

  TEST_END;
}

void circular_buffer_read_across_boundary_test() {
  TEST_START;

  uint8_t _buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
  circular_buffer_t buf = {
      .buf = _buf,
      .len = sizeof(_buf),
      .start = 2,
      .end = 1,
  };

  uint8_t dest[5];
  circular_buffer_read(dest, sizeof(dest), &buf);

  for (int i = 0; i < 4; ++i) {
    assert(dest[i] == _buf[(i + 2) % 5]);
  }

  TEST_END;
}

void circular_buffer_advance_test() {
  TEST_START;

  uint8_t _buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
  circular_buffer_t buf = {
      .buf = _buf,
      .len = sizeof(_buf),
      .start = 0,
      .end = 5,
  };

  assert(circular_buffer_len(&buf) == 5);

  circular_buffer_advance(3, &buf);

  assert(circular_buffer_len(&buf) == 2);

  TEST_END;
}

void circular_buffer_advance_across_boundary_test() {
  TEST_START;

  uint8_t _buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
  circular_buffer_t buf = {
      .buf = _buf,
      .len = sizeof(_buf),
      .start = 4,
      .end = 3,
  };

  assert(circular_buffer_len(&buf) == 4);

  circular_buffer_advance(3, &buf);

  assert(circular_buffer_len(&buf) == 1);

  TEST_END;
}
