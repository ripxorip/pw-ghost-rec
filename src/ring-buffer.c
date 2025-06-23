#include "ring-buffer.h"
#include <stdlib.h>

void ringbuffer_float_init(ringbuffer_float_t *state, uint32_t size) {
    state->size = size;
    state->start = 0;
    state->end = size - 1;
    state->buffer = (float*)malloc(sizeof(float) * size);
}

void ringbuffer_float_free(ringbuffer_float_t *state) {
    if (state->buffer) {
        free(state->buffer);
        state->buffer = NULL;
    }
}

void ringbuffer_float_increment_pointers(ringbuffer_float_t *state) {
    state->start = (state->start+1) % state->size;
    state->end = (state->end+1) % state->size;
}

void ringbuffer_float_write(ringbuffer_float_t *state, float *value) {
    state->buffer[state->start] = *value;
    ringbuffer_float_increment_pointers(state);
}

void ringbuffer_float_get_value(ringbuffer_float_t *state, float *value, int32_t offset) {
    int32_t index = state->end - offset;
    if(index < 0){
        index += state->size;
    }
    *value = state->buffer[index];
}