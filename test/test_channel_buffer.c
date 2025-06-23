#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/channel-buffer.h"

START_TEST(test_channel_buffer_init_and_free)
{
    channel_buffer_t cb;
    int sample_rate = 48000;
    int buffer_seconds = 2;
    channel_buffer_init(&cb, sample_rate, buffer_seconds);
    ck_assert_int_eq(cb.sample_rate, sample_rate);
    ck_assert_int_eq(cb.buffer_size_seconds, buffer_seconds);
    ck_assert_ptr_nonnull(cb.buffer.buffer);
    ck_assert_int_eq(cb.buffer.size, sample_rate * buffer_seconds);
    channel_buffer_free(&cb);
    ck_assert_ptr_null(cb.buffer.buffer);
}
END_TEST

START_TEST(test_channel_buffer_write_and_read)
{
    channel_buffer_t cb;
    int sample_rate = 48000;
    int buffer_seconds = 1;

    channel_buffer_init(&cb, sample_rate, buffer_seconds);

    // Write a full second of samples incrementing 1 per sample
    int num_samples = sample_rate * buffer_seconds;
    float *samples = (float *)malloc(sizeof(float) * num_samples);
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = (float)i;
        channel_buffer_write(&cb, &samples[i], 1);
    }

    float duration_seconds = 0.1f;
    float offset_seconds = 0.25f;

    int num_samples_duration = channel_buffer_duration_to_samples(&cb, duration_seconds);
    float *samples_out = (float *)malloc(sizeof(float) * num_samples_duration);

    // The most recent sample is at index num_samples - 1
    // The offset in samples from the most recent sample
    int offset_samples = (int)(offset_seconds * sample_rate);

    // The number of samples to read for the given duration
    // num_samples_duration = channel_buffer_duration_to_samples(&cb, duration_seconds);

    // The index of the first sample to read
    // Step 1: Find the index of the most recent sample
    int most_recent_index = num_samples - 1;

    // Step 2: Subtract the offset to get the starting point in the past
    int start_from_offset = most_recent_index - offset_samples;

    // Step 3: Move back by (num_samples_duration - 1) to get the first sample in the range
    int first_sample_index = start_from_offset - (num_samples_duration - 1);
    int read = channel_buffer_read(&cb, samples_out, offset_seconds, duration_seconds, num_samples_duration);

    ck_assert_int_eq(read, num_samples_duration);

    for (int i = 0; i < read; ++i) {
        float expected_value = (float)(first_sample_index + i);
        ck_assert_float_eq_tol(samples_out[i], expected_value, 1e-6);
    }

    channel_buffer_free(&cb);
    free(samples);
    free(samples_out);
}
END_TEST

START_TEST(test_channel_buffer_write_and_read_wrap_around)
{
    channel_buffer_t cb;
    int sample_rate = 48000;
    int buffer_seconds = 60; // 1 minute buffer
    channel_buffer_init(&cb, sample_rate, buffer_seconds);

    int num_samples = sample_rate * buffer_seconds;
    int total_samples = num_samples + num_samples / 2; // 1.5 wraps
    float *samples = (float *)malloc(sizeof(float) * total_samples);
    for (int i = 0; i < total_samples; ++i) {
        samples[i] = (float)i;
        channel_buffer_write(&cb, &samples[i], 1);
    }

    float duration_seconds = 0.1f;
    float offset_seconds = 0.5f;
    int num_samples_duration = channel_buffer_duration_to_samples(&cb, duration_seconds);
    float *samples_out = (float *)malloc(sizeof(float) * num_samples_duration);

    int offset_samples = (int)(offset_seconds * sample_rate);
    int read = channel_buffer_read(&cb, samples_out, offset_seconds, duration_seconds, num_samples_duration);
    ck_assert_int_eq(read, num_samples_duration);

    // The most recent sample is at index total_samples - 1
    int most_recent_index = total_samples - 1;
    int start_from_offset = most_recent_index - offset_samples;
    int first_sample_index = start_from_offset - (num_samples_duration - 1);
    for (int i = 0; i < read; ++i) {
        float expected_value = (float)(first_sample_index + i);
        ck_assert_float_eq_tol(samples_out[i], expected_value, 1e-6);
    }

    channel_buffer_free(&cb);
    free(samples);
    free(samples_out);
}
END_TEST

START_TEST(test_channel_buffer_write_blocks_and_read)
{
    channel_buffer_t cb;
    int sample_rate = 48000;
    int buffer_seconds = 60; // 1 minute buffer
    channel_buffer_init(&cb, sample_rate, buffer_seconds);

    int num_samples = sample_rate * buffer_seconds;
    float *samples = (float *)malloc(sizeof(float) * num_samples);
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = (float)i;
    }
    // Write in blocks of 128 samples
    for (int i = 0; i < num_samples; i += 128) {
        int block_size = (i + 128 <= num_samples) ? 128 : (num_samples - i);
        channel_buffer_write(&cb, &samples[i], block_size);
    }

    float duration_seconds = 0.1f;
    float offset_seconds = 0.5f;
    int num_samples_duration = channel_buffer_duration_to_samples(&cb, duration_seconds);
    float *samples_out = (float *)malloc(sizeof(float) * num_samples_duration);

    int offset_samples = (int)(offset_seconds * sample_rate);
    int read = channel_buffer_read(&cb, samples_out, offset_seconds, duration_seconds, num_samples_duration);
    ck_assert_int_eq(read, num_samples_duration);

    // The most recent sample is at index num_samples - 1
    int most_recent_index = num_samples - 1;
    int start_from_offset = most_recent_index - offset_samples;
    int first_sample_index = start_from_offset - (num_samples_duration - 1);
    for (int i = 0; i < read; ++i) {
        float expected_value = (float)(first_sample_index + i);
        ck_assert_float_eq_tol(samples_out[i], expected_value, 1e-6);
    }

    channel_buffer_free(&cb);
    free(samples);
    free(samples_out);
}
END_TEST

START_TEST(test_channel_buffer_duration_to_samples)
{
    channel_buffer_t cb;
    int sample_rate = 48000;
    int buffer_seconds = 1;
    channel_buffer_init(&cb, sample_rate, buffer_seconds);
    int samples = channel_buffer_duration_to_samples(&cb, 0.5f);
    ck_assert_int_eq(samples, 24000);
    channel_buffer_free(&cb);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("ChannelBuffer");
    TCase *tc_core = tcase_create("Core");
    SRunner *sr = srunner_create(s);

    tcase_add_test(tc_core, test_channel_buffer_init_and_free);
    tcase_add_test(tc_core, test_channel_buffer_write_and_read);
    tcase_add_test(tc_core, test_channel_buffer_write_and_read_wrap_around);
    tcase_add_test(tc_core, test_channel_buffer_write_blocks_and_read);
    tcase_add_test(tc_core, test_channel_buffer_duration_to_samples);
    suite_add_tcase(s, tc_core);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
