#ifndef RING_BUFFER
#define RING_BUFFER

#include <stddef.h>
#include <stdint.h>

// Opaque ring buffer type
struct ring_buffer;
typedef struct ring_buffer ring_buffer_t;

// Initialize the ring buffer
// duration_seconds: total seconds to store
// buffer_size: number of frames per write (stride)
// channels: number of audio channels
// sample_rate: audio sample rate (Hz)
// Returns pointer to ring buffer, or NULL on failure
ring_buffer_t *ring_buffer_init(size_t duration_seconds, size_t buffer_size, size_t channels, size_t sample_rate);

// Free the ring buffer
void ring_buffer_free(ring_buffer_t *rb);

// Write audio data to the buffer
// data: pointer to interleaved channel blocks (channels x buffer_size samples)
// Returns 0 on success, -1 on error
int ring_buffer_write(ring_buffer_t *rb, const float *data);

// Retrieve audio data from a specific time range (from seconds_ago up to seconds_ago + duration)
// out: pointer to buffer to fill (channels x frames samples)
// seconds_ago: start time offset (in seconds from now, e.g., 10.0 = 10 seconds ago)
// duration: duration in seconds to read
// Returns 0 on success, -1 on error
int ring_buffer_read(ring_buffer_t *rb, float *out, double seconds_ago, double duration);

#endif /* RING_BUFFER */
