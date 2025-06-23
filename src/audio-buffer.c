#include "audio-buffer.h"
#include <stdlib.h>

void audio_buffer_init(audio_buffer_t *ab, unsigned int num_channels, unsigned int buffer_size) {
    ab->num_channels = num_channels;
    ab->buffer_size = buffer_size;
    ab->channels = (channel_buffer_t*)malloc(sizeof(channel_buffer_t) * num_channels);
    // Optionally: loop to init each channel
}

void audio_buffer_free(audio_buffer_t *ab) {
    if (ab->channels) {
        // Optionally: loop to free each channel
        free(ab->channels);
        ab->channels = NULL;
    }
}
