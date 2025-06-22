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

struct data {
    struct pw_main_loop *loop;
    struct pw_filter *filter;
    struct pw_filter_port *in_port;
    struct pw_filter_portort *out_port;
};

static void on_process(void *userdata, struct spa_io_position *position) {
    struct data *data = (struct data *)userdata;
    float *in = pw_filter_get_dsp_buffer(data->in_port, position->clock.duration);
    float *out = pw_filter_get_dsp_buffer(data->out_port, position->clock.duration);
    uint32_t n_samples = position->clock.duration;

    if (in) {
        // Print samples to stdout (as floats)
        for (uint32_t i = 0; i < n_samples; ++i) {
            printf("%f\n", in[i]);
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
    pw_main_loop_run(data.loop);
    pw_filter_destroy(data.filter);
    pw_main_loop_destroy(data.loop);
    pw_deinit();
    return 0;
}
