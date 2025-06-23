#ifndef AUDIO_BUFFER
#define AUDIO_BUFFER

#include "channel-buffer.h"

// Forward declaration for now
typedef struct {
    channel_buffer_t *channels;
    unsigned int num_channels;
    unsigned int buffer_size;
} audio_buffer_t;

void audio_buffer_init(audio_buffer_t *ab, unsigned int num_channels, unsigned int buffer_size);
void audio_buffer_free(audio_buffer_t *ab);

#endif /* AUDIO_BUFFER */
