#include <check.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>
#include "../src/audio-buffer.h"

START_TEST(test_audio_buffer_init_and_free)
{
    audio_buffer_t ab;
    unsigned int num_channels = 2;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 1;
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);
    ck_assert_int_eq(ab.num_channels, num_channels);
    ck_assert_int_eq(ab.sample_rate, sample_rate);
    ck_assert_int_eq(ab.buffer_seconds, buffer_seconds);
    ck_assert_ptr_nonnull(ab.channels);
    audio_buffer_free(&ab);
    ck_assert_ptr_null(ab.channels);
}
END_TEST

START_TEST(test_audio_buffer_push)
{
    audio_buffer_t ab;
    unsigned int num_channels = 2;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 1;
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);
    float samples[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    audio_buffer_push(&ab, samples, 4, 0, 0); // Push to channel 0, no sync
    audio_buffer_push(&ab, samples, 4, 1, 1); // Push to channel 1, with sync
    audio_buffer_free(&ab);
}
END_TEST

START_TEST(test_audio_buffer_inject_sync)
{
    audio_buffer_t ab;
    unsigned int num_channels = 1;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 1;
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);
    float samples[32] = {0};
    // Push with sync injection
    audio_buffer_push(&ab, samples, 32, 0, 1);
    // The first 16 samples should match the sync pattern
    static const float sync_pattern[16] = {
        1.23e-5f, -2.34e-5f, 3.45e-5f, -4.56e-5f,
        5.67e-5f, -6.78e-5f, 7.89e-5f, -8.90e-5f,
        9.01e-5f, -1.23e-5f, 1.35e-5f, -2.46e-5f,
        3.57e-5f, -4.68e-5f, 5.79e-5f, -6.80e-5f
    };
    for (int i = 0; i < 16; ++i) {
        ck_assert_float_eq_tol(samples[i], sync_pattern[i], 1e-8);
    }
    audio_buffer_free(&ab);
}
END_TEST

START_TEST(test_audio_buffer_write_channel_to_wav_sine)
{
    audio_buffer_t ab;
    unsigned int num_channels = 1;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 2;
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);

    // Fill buffer with 2 seconds of 440 Hz sine wave using audio_buffer_push
    int total_samples = sample_rate * buffer_seconds;
    float *sine = (float *)malloc(sizeof(float) * total_samples);
    float freq = 440.0f;
    for (int i = 0; i < total_samples; ++i) {
        sine[i] = sinf(2.0f * 3.14159265358979323846f * freq * i / sample_rate);
    }
    for (int i = 0; i < total_samples; i += 128) {
        int block = (i + 128 <= total_samples) ? 128 : (total_samples - i);
        audio_buffer_push(&ab, &sine[i], block, 0, 0);
    }

    // Write 0.5s segment starting at 1s offset to wav
    int ret = audio_buffer_write_channel_to_wav(&ab, 0, 1.0f, 0.5f, "_out/test_sine.wav");
    ck_assert_int_eq(ret, 0);

    audio_buffer_free(&ab);
    free(sine);
}
END_TEST

START_TEST(test_audio_buffer_write_and_detect_sync)
{
    audio_buffer_t ab;
    unsigned int num_channels = 1;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 2;
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);

    // Fill buffer with 2 seconds of 440 Hz sine wave, writing 128 samples at a time using audio_buffer_push
    int total_samples = sample_rate * buffer_seconds;
    float *sine = (float *)malloc(sizeof(float) * total_samples);
    float freq = 440.0f;
    for (int i = 0; i < total_samples; ++i) {
        sine[i] = sinf(2.0f * 3.14159265358979323846f * freq * i / sample_rate);
    }
    int sync_sample = (int)(0.5f * sample_rate);
    for (int i = 0; i < total_samples; i += 128) {
        int block = (i + 128 <= total_samples) ? 128 : (total_samples - i);
        // Inject sync if sync_sample falls within this block
        if (i <= sync_sample && sync_sample < i + block) {
            audio_buffer_push(&ab, &sine[i], block, 0, 1);
        } else {
            audio_buffer_push(&ab, &sine[i], block, 0, 0);
        }
    }

    int ret = audio_buffer_write_channel_to_wav(&ab, 0, 1.6, 0.4f, "_out/test_sync.wav");
    ck_assert_int_eq(ret, 0);

    // Now open the wav and search for the sync pattern
    SF_INFO sfinfo = {0};
    SNDFILE *infile = sf_open("_out/test_sync.wav", SFM_READ, &sfinfo);
    ck_assert_ptr_nonnull(infile);
    int nsamples = sfinfo.frames;
    float *data = (float *)malloc(sizeof(float) * nsamples);
    sf_read_float(infile, data, nsamples);
    sf_close(infile);

    // The sync pattern to search for
    static const float sync_pattern[16] = {
        1.23e-5f, -2.34e-5f, 3.45e-5f, -4.56e-5f,
        5.67e-5f, -6.78e-5f, 7.89e-5f, -8.90e-5f,
        9.01e-5f, -1.23e-5f, 1.35e-5f, -2.46e-5f,
        3.57e-5f, -4.68e-5f, 5.79e-5f, -6.80e-5f
    };
    int found = 0;
    for (int i = 0; i <= nsamples - 16; ++i) {
        int match = 1;
        for (int j = 0; j < 16; ++j) {
            if (fabsf(data[i + j] - sync_pattern[j]) > 1e-6f) {
                match = 0;
                break;
            }
        }
        if (match) {
            found = 1;
            break;
        }
    }
    free(data);
    free(sine);
    audio_buffer_free(&ab);
    ck_assert(found);
}
END_TEST

START_TEST(test_audio_buffer_guitar_like_wrap_and_sync)
{
    audio_buffer_t ab;
    unsigned int num_channels = 1;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 3; // 3s buffer
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);

    // Simulate 10 seconds of guitar (sine wave)
    int total_samples = sample_rate * 10;
    float *sine = (float *)malloc(sizeof(float) * total_samples);
    float freq = 440.0f;
    for (int i = 0; i < total_samples; ++i) {
        sine[i] = sinf(2.0f * 3.14159265358979323846f * freq * i / sample_rate);
    }

    // Push in blocks, inject sync at 2s before the end (i.e., at 8.0s in the 10s stream)
    int sync_sample = sample_rate * 8; // 8.0s into the stream
    for (int i = 0; i < total_samples; i += 128) {
        int block = (i + 128 <= total_samples) ? 128 : (total_samples - i);
        if (i <= sync_sample && sync_sample < i + block) {
            audio_buffer_push(&ab, &sine[i], block, 0, 1);
        } else {
            audio_buffer_push(&ab, &sine[i], block, 0, 0);
        }
    }

    // Read out 0.8s, starting 2.3s before the end (i.e., offset=2.3, duration=0.8)
    int ret = audio_buffer_write_channel_to_wav(&ab, 0, 2.0f, 0.8f, "_out/test_guitar_sync.wav");
    ck_assert_int_eq(ret, 0);

    // Now open the wav and search for the sync pattern
    SF_INFO sfinfo = {0};
    SNDFILE *infile = sf_open("_out/test_guitar_sync.wav", SFM_READ, &sfinfo);
    ck_assert_ptr_nonnull(infile);
    int nsamples = sfinfo.frames;
    float *data = (float *)malloc(sizeof(float) * nsamples);
    sf_read_float(infile, data, nsamples);
    sf_close(infile);

    // The sync pattern to search for
    static const float sync_pattern[16] = {
        1.23e-5f, -2.34e-5f, 3.45e-5f, -4.56e-5f,
        5.67e-5f, -6.78e-5f, 7.89e-5f, -8.90e-5f,
        9.01e-5f, -1.23e-5f, 1.35e-5f, -2.46e-5f,
        3.57e-5f, -4.68e-5f, 5.79e-5f, -6.80e-5f
    };
    int found = 0;
    for (int i = 0; i <= nsamples - 16; ++i) {
        int match = 1;
        for (int j = 0; j < 16; ++j) {
            if (fabsf(data[i + j] - sync_pattern[j]) > 1e-6f) {
                match = 0;
                break;
            }
        }
        if (match) {
            found = 1;
            break;
        }
    }
    free(data);
    free(sine);
    audio_buffer_free(&ab);
    ck_assert(found);
}
END_TEST

START_TEST(test_audio_buffer_sync_at_wrap_boundary)
{
    audio_buffer_t ab;
    unsigned int num_channels = 1;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 30 * 60; // 30 minutes
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);

    // The buffer size in samples
    int buffer_samples = sample_rate * buffer_seconds;
    // We'll fill the buffer with a ramp, wrapping once
    float *ramp = (float *)malloc(sizeof(float) * buffer_samples);
    for (int i = 0; i < buffer_samples; ++i) {
        ramp[i] = (float)i / (float)buffer_samples;
    }

    // Fill the buffer completely (simulate long stream)
    for (int i = 0; i < buffer_samples; i += 1024) {
        int block = (i + 1024 <= buffer_samples) ? 1024 : (buffer_samples - i);
        audio_buffer_push(&ab, &ramp[i], block, 0, 0);
    }

    // Now inject sync pattern exactly at the wrap boundary (i.e., next push will wrap)
    // We'll push a block that starts at the end and wraps to the start
    int sync_block_size = 32; // must be > sync pattern size
    float *sync_block = (float *)malloc(sizeof(float) * sync_block_size);
    for (int i = 0; i < sync_block_size; ++i) {
        sync_block[i] = 0.0f;
    }
    audio_buffer_push(&ab, sync_block, sync_block_size, 0, 1); // inject sync

    // Now export a segment that includes the wrap boundary (last 2*sync_block_size samples)
    float offset = (float)(sync_block_size * 2) / (float)sample_rate; // correct offset to include wrap boundary
    float duration = 1.0f; // 1 second duration for easier inspection in Reaper
    int ret = audio_buffer_write_channel_to_wav(&ab, 0, offset, duration, "_out/test_sync_wrap_boundary.wav");
    ck_assert_int_eq(ret, 0);

    // Open the wav and search for the sync pattern
    SF_INFO sfinfo = {0};
    SNDFILE *infile = sf_open("_out/test_sync_wrap_boundary.wav", SFM_READ, &sfinfo);
    ck_assert_ptr_nonnull(infile);
    int nsamples = sfinfo.frames;
    float *data = (float *)malloc(sizeof(float) * nsamples);
    sf_read_float(infile, data, nsamples);
    sf_close(infile);

    static const float sync_pattern[16] = {
        1.23e-5f, -2.34e-5f, 3.45e-5f, -4.56e-5f,
        5.67e-5f, -6.78e-5f, 7.89e-5f, -8.90e-5f,
        9.01e-5f, -1.23e-5f, 1.35e-5f, -2.46e-5f,
        3.57e-5f, -4.68e-5f, 5.79e-5f, -6.80e-5f
    };
    int found = 0;
    for (int i = 0; i <= nsamples - 16; ++i) {
        int match = 1;
        for (int j = 0; j < 16; ++j) {
            if (fabsf(data[i + j] - sync_pattern[j]) > 1e-6f) {
                match = 0;
                break;
            }
        }
        if (match) {
            found = 1;
            break;
        }
    }
    free(data);
    free(ramp);
    free(sync_block);
    audio_buffer_free(&ab);
    ck_assert(found);
}
END_TEST

START_TEST(test_audio_buffer_offset_from_sync_feature)
{
    audio_buffer_t ab;
    unsigned int num_channels = 1;
    unsigned int sample_rate = 48000;
    unsigned int buffer_seconds = 2;
    audio_buffer_init(&ab, num_channels, sample_rate, buffer_seconds);

    // Fill buffer with 2 seconds of 440 Hz sine wave using audio_buffer_push in 128-sample blocks
    int total_samples = sample_rate * buffer_seconds;
    float *sine = (float *)malloc(sizeof(float) * total_samples);
    float freq = 440.0f;
    for (int i = 0; i < total_samples; ++i) {
        sine[i] = sinf(2.0f * 3.14159265358979323846f * freq * i / sample_rate);
    }
    int sync_sample = (int)(0.5f * sample_rate);
    for (int i = 0; i < total_samples; i += 128) {
        int block = (i + 128 <= total_samples) ? 128 : (total_samples - i);
        // Inject sync if sync_sample falls within this block
        if (i <= sync_sample && sync_sample < i + block) {
            audio_buffer_push(&ab, &sine[i], block, 0, 1);
        } else {
            audio_buffer_push(&ab, &sine[i], block, 0, 0);
        }
    }

    // Use the new feature to get the offset (time since sync)
    float offset = audio_buffer_seconds_since_sync(&ab);
    ck_assert(offset > 0.0f);
    // Add 0.1s to offset, subtract 0.1s for duration (ensure duration is positive)
    float write_offset = offset + 0.1f;
    float write_duration = (offset - 0.1f > 0.0f) ? (offset - 0.1f) : 0.1f;

    int ret = audio_buffer_write_channel_to_wav(&ab, 0, write_offset, write_duration, "_out/test_offset_from_sync_feature.wav");
    ck_assert_int_eq(ret, 0);

    // Now open the wav and search for the sync pattern
    SF_INFO sfinfo = {0};
    SNDFILE *infile = sf_open("_out/test_offset_from_sync_feature.wav", SFM_READ, &sfinfo);
    ck_assert_ptr_nonnull(infile);
    int nsamples = sfinfo.frames;
    float *data = (float *)malloc(sizeof(float) * nsamples);
    sf_read_float(infile, data, nsamples);
    sf_close(infile);

    static const float sync_pattern[16] = {
        1.23e-5f, -2.34e-5f, 3.45e-5f, -4.56e-5f,
        5.67e-5f, -6.78e-5f, 7.89e-5f, -8.90e-5f,
        9.01e-5f, -1.23e-5f, 1.35e-5f, -2.46e-5f,
        3.57e-5f, -4.68e-5f, 5.79e-5f, -6.80e-5f
    };
    int found = 0;
    for (int i = 0; i <= nsamples - 16; ++i) {
        int match = 1;
        for (int j = 0; j < 16; ++j) {
            if (fabsf(data[i + j] - sync_pattern[j]) > 1e-6f) {
                match = 0;
                break;
            }
        }
        if (match) {
            found = 1;
            break;
        }
    }
    free(data);
    free(sine);
    audio_buffer_free(&ab);
    ck_assert(found);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("AudioBuffer");
    TCase *tc_core = tcase_create("Core");
    SRunner *sr = srunner_create(s);

    tcase_add_test(tc_core, test_audio_buffer_init_and_free);
    tcase_add_test(tc_core, test_audio_buffer_push);
    tcase_add_test(tc_core, test_audio_buffer_inject_sync);
    tcase_add_test(tc_core, test_audio_buffer_write_channel_to_wav_sine);
    tcase_add_test(tc_core, test_audio_buffer_write_and_detect_sync);
    tcase_add_test(tc_core, test_audio_buffer_guitar_like_wrap_and_sync);
    tcase_add_test(tc_core, test_audio_buffer_sync_at_wrap_boundary);
    tcase_add_test(tc_core, test_audio_buffer_offset_from_sync_feature);
    suite_add_tcase(s, tc_core);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
