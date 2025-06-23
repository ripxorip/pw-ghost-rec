#ifndef CHANNEL_BUFFER
#define CHANNEL_BUFFER

#include "ring-buffer.h"

typedef struct {
    ringbuffer_float_t buffer;
    int sample_rate;
    int buffer_size_seconds;
} channel_buffer_t;

void channel_buffer_init(channel_buffer_t *cb, int sample_rate, int buffer_size_seconds);
void channel_buffer_free(channel_buffer_t *cb);

void channel_buffer_write(channel_buffer_t *cb, const float *samples, int number_of_samples);

int channel_buffer_duration_to_samples(const channel_buffer_t *cb, float duration_seconds);

int channel_buffer_read(const channel_buffer_t *cb, float *samples, float offset_seconds, float duration_seconds, int samples_size);

#endif /* CHANNEL_BUFFER */
