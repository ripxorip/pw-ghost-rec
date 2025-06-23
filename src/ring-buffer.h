#ifndef RING_BUFFER
#define RING_BUFFER

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    float *buffer;
    uint32_t start;
    uint32_t end;
    uint32_t size;
} ringbuffer_float_t;

void ringbuffer_float_init(ringbuffer_float_t *state, uint32_t size);

void ringbuffer_float_free(ringbuffer_float_t *state);

void ringbuffer_float_increment_pointers(ringbuffer_float_t *state);

void ringbuffer_float_write(ringbuffer_float_t *state, float *value);

void ringbuffer_float_get_value(ringbuffer_float_t *state, float *value, int32_t offset);


#endif /* RING_BUFFER */
