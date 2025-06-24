/*
 * pw-ghost-rec.c - PipeWire audio capture and local sample logging
 *
 * (c) 2025 Philip K. Gisslow
 * This file is part of the pw-ghost-rec project.
 *
 * This tool captures audio samples from a PipeWire input and prints them to stdout.
 * It is intended to be extended to save a local copy of the audio data for later use.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pipewire/pipewire.h>
#include <pipewire/filter.h>
#include <lo/lo.h>
#include <pthread.h>
#include <stdatomic.h>
#include "audio-buffer.h"

#define AUDIO_BUFFER_SECONDS (30 * 60)
#define SYNC_PRE_DELAY_SECONDS 0.100

struct data {
    struct pw_main_loop *loop;
    struct pw_filter *filter;
    struct pw_filter_port *in_port;
    struct pw_filter_portort *out_port;
    audio_buffer_t *audio_buffer;
    int audio_buffer_initialized;
    pthread_mutex_t buffer_mutex;
    int pending_sync_inject;
    int buffer_write_in_progress;
};

// Add a global atomic flag to signal shutdown
static atomic_int osc_should_exit = 0;

static void on_process(void *userdata, struct spa_io_position *position) {
    struct data *data = (struct data *)userdata;
    float *in = pw_filter_get_dsp_buffer(data->in_port, position->clock.duration);
    float *out = pw_filter_get_dsp_buffer(data->out_port, position->clock.duration);
    uint32_t n_samples = position->clock.duration;
    static float sync_delay_accum = 0.0f;
    static int waiting_for_sync = 0;

    // Lazy audio_buffer_t initialization
    if (!data->audio_buffer_initialized && in) {
        uint32_t sample_rate = 48000; // default
        if (position && position->clock.rate.denom > 0) {
            if (position->clock.rate.num == 1) {
                sample_rate = position->clock.rate.denom;
            } else if (position->clock.rate.num > 0) {
                sample_rate = position->clock.rate.num / position->clock.rate.denom;
            }
        }
        data->audio_buffer = malloc(sizeof(audio_buffer_t));
        audio_buffer_init(data->audio_buffer, 1, sample_rate, AUDIO_BUFFER_SECONDS);
        printf("Initialized audio buffer with sample rate %u, length %d seconds\n", sample_rate, AUDIO_BUFFER_SECONDS);
        pthread_mutex_init(&data->buffer_mutex, NULL);
        data->audio_buffer_initialized = 1;
        data->pending_sync_inject = 0;
        data->buffer_write_in_progress = 0;
    }

    if (in) {
        // Write to audio buffer if initialized
        if (data->audio_buffer_initialized && !data->buffer_write_in_progress) {
            int inject_sync = 0;
            if (waiting_for_sync) {
                sync_delay_accum += (float)n_samples / (float)data->audio_buffer->sample_rate;
                if (sync_delay_accum >= SYNC_PRE_DELAY_SECONDS) {
                    inject_sync = 1;
                    waiting_for_sync = 0;
                    sync_delay_accum = 0.0f;
                }
            } else if (data->pending_sync_inject) {
                waiting_for_sync = 1;
                sync_delay_accum = 0.0f;
                data->pending_sync_inject = 0;
            }
            pthread_mutex_lock(&data->buffer_mutex);
            audio_buffer_push(data->audio_buffer, in, n_samples, 0, inject_sync);
            pthread_mutex_unlock(&data->buffer_mutex);
        }
    }

    if (in && out) {
        // Passthrough: copy input to output
        for (uint32_t i = 0; i < n_samples; ++i) {
            out[i] = in[i];
        }
    } else if (out) {
        // Output is available but input is not: zero the output
        for (uint32_t i = 0; i < n_samples; ++i) {
            out[i] = 0.0f;
        }
    }
    // If neither in nor out, do nothing
}

static const struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = on_process,
};

static void do_quit(void *userdata, int signal_number) {
    (void)signal_number; // Unused parameter
    struct data *data = (struct data *)userdata;
    pw_main_loop_quit(data->loop);
}

// Worker thread to write buffer to wav file
void *write_buffer_thread(void *arg) {
    struct data *data = (struct data *)arg;
    pthread_mutex_lock(&data->buffer_mutex);
    data->buffer_write_in_progress = 1;
    // Write the buffer to wav (mono, all samples)

    float time_since_sync = audio_buffer_seconds_since_sync(data->audio_buffer);
    float pre_time = 0.1f;
    float offset = time_since_sync + pre_time;
    float duration = time_since_sync - pre_time;
    if (duration < 0.01f) duration = 0.01f; // Clamp to minimum duration
    audio_buffer_write_channel_to_wav(data->audio_buffer, 0, offset, duration, "/tmp/pw-ghost-buffer.wav");
    data->buffer_write_in_progress = 0;
    pthread_mutex_unlock(&data->buffer_mutex);
    return NULL;
}

// OSC handler for /record
int osc_record(const char *path, const char *types, lo_arg **argv,
              int argc, struct lo_message_ *msg, void *user_data) {
    struct data *data = (struct data *)user_data;
    (void)path; (void)types; (void)msg;
    if (argc == 1 && types && types[0] == 'f') {
        float val = argv[0]->f;
        printf("OSC: Received /record (float): %f\n", val);
        if (val == 1.0f) {
            data->pending_sync_inject = 1;
        } else if (val == 0.0f) {
            if (!data->buffer_write_in_progress) {
                pthread_t writer;
                pthread_create(&writer, NULL, write_buffer_thread, data);
                pthread_detach(writer);
            }
        }
    }
    return 0;
}

void *osc_server_thread(void *arg) {
    struct data *data = (struct data *)arg;
    lo_server_thread st = lo_server_thread_new("9000", NULL);
    lo_server_thread_add_method(st, "/record", NULL, osc_record, data);
    lo_server_thread_start(st);
    while (!atomic_load(&osc_should_exit)) {
        sleep(1);
    }
    lo_server_thread_stop(st);
    lo_server_thread_free(st);
    return NULL;
}

int main(int argc, char *argv[]) {
    struct data data;
    memset(&data, 0, sizeof(data));
    pw_init(&argc, &argv);
    data.loop = pw_main_loop_new(NULL);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);
    data.filter = pw_filter_new_simple(
        pw_main_loop_get_loop(data.loop),
        "pw-ghost-rec",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Filter",
            PW_KEY_MEDIA_ROLE, "DSP",
            NULL),
        &filter_events,
        &data);
    data.in_port = pw_filter_add_port(data.filter,
        PW_DIRECTION_INPUT,
        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        0,
        pw_properties_new(
            PW_KEY_FORMAT_DSP, "32 bit float mono audio",
            PW_KEY_PORT_NAME, "input",
            NULL),
        NULL, 0);
    data.out_port = pw_filter_add_port(data.filter,
        PW_DIRECTION_OUTPUT,
        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        0,
        pw_properties_new(
            PW_KEY_FORMAT_DSP, "32 bit float mono audio",
            PW_KEY_PORT_NAME, "output-right",
            NULL),
        NULL, 0);
    if (pw_filter_connect(data.filter,
            PW_FILTER_FLAG_RT_PROCESS,
            NULL, 0) < 0) {
        fprintf(stderr, "can't connect\n");
        return -1;
    }
    pthread_t osc_thread;
    pthread_create(&osc_thread, NULL, osc_server_thread, &data);
    pw_main_loop_run(data.loop);
    // Signal OSC thread to exit
    atomic_store(&osc_should_exit, 1);
    pthread_join(osc_thread, NULL);
    pw_filter_destroy(data.filter);
    pw_main_loop_destroy(data.loop);
    pw_deinit();
    return 0;
}
