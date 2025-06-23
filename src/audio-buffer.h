#ifndef AUDIO_BUFFER
#define AUDIO_BUFFER

#include "channel-buffer.h"

// Forward declaration for now
typedef struct {
    channel_buffer_t *channels;
    unsigned int num_channels;
    unsigned int sample_rate;
    unsigned int buffer_seconds;
    int samples_since_sync; // -1 if no sync injected
    int sync_active;        // 1 if sync injected, 0 otherwise
} audio_buffer_t;

void audio_buffer_init(audio_buffer_t *ab, unsigned int num_channels, unsigned int sample_rate, unsigned int buffer_seconds);
void audio_buffer_free(audio_buffer_t *ab);
void audio_buffer_push(audio_buffer_t *ab, const float *samples, int num_samples, int channel, int inject_sync);

// Write a segment of a channel to a wav file
int audio_buffer_write_channel_to_wav(audio_buffer_t *ab, int channel, float offset_seconds, float duration_seconds, const char *filename);

// Returns seconds since last sync injection, or -1.0f if no sync
float audio_buffer_seconds_since_sync(const audio_buffer_t *ab);

// Call this to stop sync tracking (e.g., on stop OSC)
void audio_buffer_stop_sync(audio_buffer_t *ab);

#endif /* AUDIO_BUFFER */
