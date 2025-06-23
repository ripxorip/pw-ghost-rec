#include "channel-buffer.h"
#include <string.h>

void channel_buffer_init(channel_buffer_t *cb, int sample_rate, int buffer_size_seconds) {
    cb->sample_rate = sample_rate;
    cb->buffer_size_seconds = buffer_size_seconds;
    int buffer_size = sample_rate * buffer_size_seconds;
    ringbuffer_float_init(&cb->buffer, buffer_size);
}

void channel_buffer_free(channel_buffer_t *cb) {
    ringbuffer_float_free(&cb->buffer);
}

void channel_buffer_write(channel_buffer_t *cb, const float *samples, int number_of_samples) {
    for (int i = 0; i < number_of_samples; ++i) {
        // Cast away const since ringbuffer_float_write does not modify the input value
        ringbuffer_float_write(&cb->buffer, (float *)&samples[i]);
    }
}

int channel_buffer_duration_to_samples(const channel_buffer_t *cb, float duration_seconds) {
    return (int)(duration_seconds * cb->sample_rate);
}

int channel_buffer_read(const channel_buffer_t *cb, float *samples, float offset_seconds, float duration_seconds, int samples_size) {
    int offset_samples = (int)(offset_seconds * cb->sample_rate);
    int num_samples = (int)(duration_seconds * cb->sample_rate);
    if (num_samples > samples_size) num_samples = samples_size;
    // Read forward in time from (now - offset) for duration seconds
    for (int i = 0; i < num_samples; ++i) {
        ringbuffer_float_get_value((ringbuffer_float_t *)&cb->buffer, &samples[i], offset_samples - i);
    }
    return num_samples;
}
