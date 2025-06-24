#include "audio-buffer.h"
#include <stdlib.h>
#include <sndfile.h>

void audio_buffer_init(audio_buffer_t *ab, unsigned int num_channels, unsigned int sample_rate, unsigned int buffer_seconds) {
    ab->num_channels = num_channels;
    ab->sample_rate = sample_rate;
    ab->buffer_seconds = buffer_seconds;
    ab->channels = (channel_buffer_t*)malloc(sizeof(channel_buffer_t) * num_channels);
    ab->samples_since_sync = -1;
    ab->sync_active = 0;
    for (unsigned int i = 0; i < num_channels; ++i) {
        channel_buffer_init(&ab->channels[i], sample_rate, buffer_seconds);
    }
}

void audio_buffer_free(audio_buffer_t *ab) {
    if (ab->channels) {
        for (unsigned int i = 0; i < ab->num_channels; ++i) {
            channel_buffer_free(&ab->channels[i]);
        }
        free(ab->channels);
        ab->channels = NULL;
    }
}

static void inject_sync(float *samples, int num_samples) {
    // Use a very low amplitude, non-musical, pseudo-random pattern
    static const float sync_pattern[16] = {
        1.23e-5f, -2.34e-5f, 3.45e-5f, -4.56e-5f,
        5.67e-5f, -6.78e-5f, 7.89e-5f, -8.90e-5f,
        9.01e-5f, -1.23e-5f, 1.35e-5f, -2.46e-5f,
        3.57e-5f, -4.68e-5f, 5.79e-5f, -6.80e-5f
    };
    int n = (num_samples < 16) ? num_samples : 16;
    for (int i = 0; i < n; ++i) {
        samples[i] = sync_pattern[i];
    }
}

void audio_buffer_push(audio_buffer_t *ab, float *samples, int num_samples, int channel, int inject_sync_flag) {
    if (ab == NULL || ab->channels == NULL || channel < 0 || channel >= (int)ab->num_channels) return;
    if (inject_sync_flag) {
        inject_sync(samples, num_samples);
        ab->samples_since_sync = 0;
        ab->sync_active = 1;
    } else if (ab->sync_active) {
        ab->samples_since_sync += num_samples;
    }
    channel_buffer_write(&ab->channels[channel], samples, num_samples);
}

int audio_buffer_write_channel_to_wav(audio_buffer_t *ab, int channel, float offset_seconds, float duration_seconds, const char *filename) {
    if (!ab || !ab->channels || channel < 0 || channel >= (int)ab->num_channels) return -1;
    int sample_rate = ab->sample_rate;
    int num_samples = (int)(duration_seconds * sample_rate);
    float *buffer = (float *)malloc(sizeof(float) * num_samples);
    if (!buffer) return -2;
    int read = channel_buffer_read(&ab->channels[channel], buffer, offset_seconds, duration_seconds, num_samples);
    if (read <= 0) { free(buffer); return -3; }

    // Soft clamp all float audio to [-1.0, +1.0] before writing to WAV
    for (int i = 0; i < read; ++i) {
        if (buffer[i] > 1.0f) buffer[i] = 0.99f;
        else if (buffer[i] < -1.0f) buffer[i] = -0.99f;
    }

    SF_INFO sfinfo = {0};
    sfinfo.samplerate = sample_rate;
    sfinfo.frames = read;
    sfinfo.channels = 1;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    SNDFILE *outfile = sf_open(filename, SFM_WRITE, &sfinfo);
    if (!outfile) { free(buffer); return -4; }
    sf_write_float(outfile, buffer, read);
    sf_close(outfile);
    free(buffer);
    return 0;
}

float audio_buffer_seconds_since_sync(const audio_buffer_t *ab) {
    if (!ab || !ab->sync_active || ab->samples_since_sync < 0) return -1.0f;
    return (float)ab->samples_since_sync / (float)ab->sample_rate;
}

void audio_buffer_stop_sync(audio_buffer_t *ab) {
    if (!ab) return;
    ab->sync_active = 0;
    ab->samples_since_sync = -1;
}
