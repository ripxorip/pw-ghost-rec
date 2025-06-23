#include "ring-buffer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Internal ring buffer structure
typedef struct ring_buffer {
    float *data; // Interleaved channel blocks
    size_t total_frames; // Total frames in buffer
    size_t buffer_size;  // Frames per write/read
    size_t channels;
    size_t sample_rate;
    size_t write_index; // Current write position (in frames)
    size_t num_blocks;  // How many blocks fit in buffer
    int filled;         // Has the buffer wrapped at least once?
} ring_buffer_t;

ring_buffer_t *ring_buffer_init(size_t duration_seconds, size_t buffer_size, size_t channels, size_t sample_rate) {
    if (duration_seconds == 0 || buffer_size == 0 || channels == 0 || sample_rate == 0) return NULL;
    size_t total_frames = duration_seconds * sample_rate;
    size_t num_blocks = total_frames / buffer_size;
    if (num_blocks == 0) num_blocks = 1;
    ring_buffer_t *rb = calloc(1, sizeof(ring_buffer_t));
    if (!rb) return NULL;
    rb->channels = channels;
    rb->buffer_size = buffer_size;
    rb->sample_rate = sample_rate;
    rb->num_blocks = num_blocks;
    rb->total_frames = num_blocks * buffer_size;
    rb->data = calloc(rb->num_blocks * rb->channels * rb->buffer_size, sizeof(float));
    if (!rb->data) { free(rb); return NULL; }
    rb->write_index = 0;
    rb->filled = 0;
    return rb;
}

void ring_buffer_free(ring_buffer_t *rb) {
    if (!rb) return;
    free(rb->data);
    free(rb);
}

int ring_buffer_write(ring_buffer_t *rb, const float *data) {
    if (!rb || !data) return -1;
    size_t offset = rb->write_index * rb->channels * rb->buffer_size;
    memcpy(&rb->data[offset], data, sizeof(float) * rb->channels * rb->buffer_size);
    rb->write_index = (rb->write_index + 1) % rb->num_blocks;
    if (rb->write_index == 0) rb->filled = 1;
    return 0;
}

// Updated read function: reads a range from seconds_ago up to seconds_ago + duration
int ring_buffer_read(ring_buffer_t *rb, float *out, double seconds_ago, double duration) {
    if (!rb || !out || duration <= 0) return -1;
    size_t start_frame_ago = (size_t)round(seconds_ago * rb->sample_rate);
    size_t frames_to_read = (size_t)round(duration * rb->sample_rate);

    size_t available_frames = rb->filled ? rb->total_frames : rb->write_index * rb->buffer_size;
    if (start_frame_ago + frames_to_read > available_frames) return -1; // Not enough data

    // The absolute index of the most recent sample written
    size_t total_samples_written = rb->filled
        ? rb->total_frames + (rb->write_index * rb->buffer_size)
        : rb->write_index * rb->buffer_size;

    // The absolute index of the first sample to read
    size_t wanted_sample_index = total_samples_written - start_frame_ago - frames_to_read;

    // The ring buffer index to start reading from
    size_t ring_index = wanted_sample_index % rb->total_frames;

    // Copy frames, handling wrap-around
    size_t frames_left = frames_to_read;
    size_t out_offset = 0;
    size_t start_frame = ring_index;
    while (frames_left > 0) {
        size_t chunk = rb->total_frames - (start_frame % rb->total_frames);
        if (chunk > frames_left) chunk = frames_left;
        size_t src_offset = (start_frame % rb->total_frames) * rb->channels;
        memcpy(&out[out_offset], &rb->data[src_offset], sizeof(float) * chunk * rb->channels);
        out_offset += chunk * rb->channels;
        start_frame = (start_frame + chunk) % rb->total_frames;
        frames_left -= chunk;
    }
    return 0;
}