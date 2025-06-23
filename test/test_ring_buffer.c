#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/ring-buffer.h"

START_TEST(test_ring_buffer_init_params)
{
    // Invalid params
    ck_assert_ptr_eq(ring_buffer_init(0, 128, 2, 48000), NULL);
    ck_assert_ptr_eq(ring_buffer_init(10, 0, 2, 48000), NULL);
    ck_assert_ptr_eq(ring_buffer_init(10, 128, 0, 48000), NULL);
    ck_assert_ptr_eq(ring_buffer_init(10, 128, 2, 0), NULL);
    // Valid params
    ring_buffer_t *rb = ring_buffer_init(1, 128, 2, 48000);
    ck_assert_ptr_ne(rb, NULL);
    ring_buffer_free(rb);
}
END_TEST

START_TEST(test_ring_buffer_write_read_basic)
{
    size_t duration = 2; // seconds
    size_t buffer_size = 4;
    size_t channels = 2;
    size_t sample_rate = 8;
    ring_buffer_t *rb = ring_buffer_init(duration, buffer_size, channels, sample_rate);
    ck_assert_ptr_ne(rb, NULL);
    float in[buffer_size * channels];
    float out[buffer_size * channels];
    // Fill with known pattern and write
    for (size_t i = 0; i < buffer_size * channels; ++i) in[i] = (float)i;
    ck_assert_int_eq(ring_buffer_write(rb, in), 0);
    // Read most recent
    ck_assert_int_eq(ring_buffer_read(rb, out, 0.0, (float)buffer_size / sample_rate), 0);
    for (size_t i = 0; i < buffer_size * channels; ++i) ck_assert_float_eq(out[i], in[i]);
    ring_buffer_free(rb);
}
END_TEST

START_TEST(test_ring_buffer_wrap_around)
{
    size_t duration = 1; // second
    size_t buffer_size = 2;
    size_t channels = 1;
    size_t sample_rate = 4;
    ring_buffer_t *rb = ring_buffer_init(duration, buffer_size, channels, sample_rate);
    ck_assert_ptr_ne(rb, NULL);
    float in[buffer_size * channels];
    float out[buffer_size * channels];
    // Write enough to wrap
    for (int w = 0; w < 4; ++w) {
        for (size_t i = 0; i < buffer_size * channels; ++i) in[i] = (float)(w * 10 + i);
        ck_assert_int_eq(ring_buffer_write(rb, in), 0);
    }
    // Read oldest (should be 3rd write, i.e. [20, 21])
    ck_assert_int_eq(ring_buffer_read(rb, out, 0.5, 0.5), 0);
    for (size_t i = 0; i < buffer_size * channels; ++i) ck_assert_float_eq(out[i], 20 + i);
    // Read most recent (should be 4th write, i.e. [30, 31])
    ck_assert_int_eq(ring_buffer_read(rb, out, 0.0, 0.5), 0);
    for (size_t i = 0; i < buffer_size * channels; ++i) ck_assert_float_eq(out[i], 30 + i);
    ring_buffer_free(rb);
}
END_TEST

START_TEST(test_ring_buffer_wrap_around_increasing)
{
    size_t duration = 2; // seconds
    size_t buffer_size = 128;
    size_t channels = 1;
    size_t sample_rate = 48000;
    ring_buffer_t *rb = ring_buffer_init(duration, buffer_size, channels, sample_rate);
    ck_assert_ptr_ne(rb, NULL);
    float in[buffer_size * channels];
    float out[(size_t)(0.25 * sample_rate) * channels];

    size_t blocks_in_buffer = (duration * sample_rate) / buffer_size;
    size_t blocks_to_write = blocks_in_buffer + 2; // fill and wrap
    for (size_t w = 0; w < blocks_to_write; ++w) {
        for (size_t i = 0; i < buffer_size * channels; ++i) {
            in[i] = (float)(w * buffer_size + i);
        }
        ck_assert_int_eq(ring_buffer_write(rb, in), 0);
    }

    // Read 0.5s ago, duration 0.25s
    double seconds_ago = 0.5;
    double duration_s = 0.25;
    size_t frames_to_read = (size_t)(duration_s * sample_rate);
    ck_assert_int_eq(ring_buffer_read(rb, out, seconds_ago, duration_s), 0);

    // Print all the samples for debugging
    for (size_t i = 0; i < frames_to_read; ++i) {
        printf("out[%zu] = %f\n", i, out[i]);
    }

    // Calculate expected start sample index
    size_t total_samples_written = blocks_to_write * buffer_size;
    size_t start_sample = total_samples_written - (size_t)(seconds_ago * sample_rate);
    // Print debug info
    printf("total_samples_written = %zu\n", total_samples_written);
    size_t buffer_capacity = duration * sample_rate;
    printf("buffer_capacity = %zu\n", buffer_capacity);
    size_t oldest_sample_index = total_samples_written - buffer_capacity;
    printf("oldest_sample_index = %zu\n", oldest_sample_index);
    printf("frames_to_read = %zu\n", frames_to_read);
    printf("start_sample = %zu\n", start_sample);
    printf("first out[0] = %f\n", out[0]);
    for (size_t i = 0; i < frames_to_read; ++i) {
        ck_assert_float_eq(out[i], (float)(start_sample + i));
    }
    ring_buffer_free(rb);
}
END_TEST

START_TEST(test_ring_buffer_not_filled)
{
    size_t duration = 1;
    size_t buffer_size = 2;
    size_t channels = 1;
    size_t sample_rate = 4;
    ring_buffer_t *rb = ring_buffer_init(duration, buffer_size, channels, sample_rate);
    float in[buffer_size * channels];
    float out[buffer_size * channels];
    for (size_t i = 0; i < buffer_size * channels; ++i) in[i] = (float)i;
    ck_assert_int_eq(ring_buffer_write(rb, in), 0);
    // Try to read more than written (should fail)
    ck_assert_int_eq(ring_buffer_read(rb, out, 0.5, 0.5), -1);
    ring_buffer_free(rb);
}
END_TEST

START_TEST(test_ring_buffer_out_of_bounds)
{
    size_t duration = 1;
    size_t buffer_size = 2;
    size_t channels = 1;
    size_t sample_rate = 4;
    ring_buffer_t *rb = ring_buffer_init(duration, buffer_size, channels, sample_rate);
    float in[buffer_size * channels];
    float out[buffer_size * channels * 2];
    for (size_t i = 0; i < buffer_size * channels; ++i) in[i] = (float)i;
    ck_assert_int_eq(ring_buffer_write(rb, in), 0);
    // Out of range: too far in past
    ck_assert_int_eq(ring_buffer_read(rb, out, 2.0, 0.5), -1);
    // Out of range: duration too long
    ck_assert_int_eq(ring_buffer_read(rb, out, 0.0, 2.0), -1);
    ring_buffer_free(rb);
}
END_TEST

START_TEST(test_ring_buffer_two_channels)
{
    size_t duration = 1; // second
    size_t buffer_size = 2;
    size_t channels = 2;
    size_t sample_rate = 4;
    ring_buffer_t *rb = ring_buffer_init(duration, buffer_size, channels, sample_rate);
    ck_assert_ptr_ne(rb, NULL);
    float in[buffer_size * channels];
    float out[buffer_size * channels];
    // Write two blocks with different values for each channel
    for (int w = 0; w < 2; ++w) {
        for (size_t i = 0; i < buffer_size; ++i) {
            in[i * channels + 0] = (float)(w * 100 + i); // channel 0
            in[i * channels + 1] = (float)(w * 1000 + i); // channel 1
        }
        ck_assert_int_eq(ring_buffer_write(rb, in), 0);
    }
    // Read most recent block
    ck_assert_int_eq(ring_buffer_read(rb, out, 0.0, 0.5), 0);
    for (size_t i = 0; i < buffer_size; ++i) {
        ck_assert_float_eq(out[i * channels + 0], 100 + i);
        ck_assert_float_eq(out[i * channels + 1], 1000 + i);
    }
    // Read previous block
    ck_assert_int_eq(ring_buffer_read(rb, out, 0.5, 0.5), 0);
    for (size_t i = 0; i < buffer_size; ++i) {
        ck_assert_float_eq(out[i * channels + 0], 0 + i);
        ck_assert_float_eq(out[i * channels + 1], 0 + i);
    }
    ring_buffer_free(rb);
}
END_TEST

START_TEST(test_ring_buffer_real_life)
{
    size_t sample_rate = 48000;
    size_t buffer_size = 128;
    size_t duration_seconds = 10 * 60; // 10 minutes
    size_t channels = 1;
    ring_buffer_t *rb = ring_buffer_init(duration_seconds, buffer_size, channels, sample_rate);
    ck_assert_ptr_ne(rb, NULL);

    // Write 15 minutes of data (wraps around)
    size_t total_write_seconds = 15 * 60;
    size_t total_frames = total_write_seconds * sample_rate;
    size_t total_blocks = total_frames / buffer_size;
    float *in = malloc(buffer_size * channels * sizeof(float));
    ck_assert_ptr_ne(in, NULL);
    // Pattern: value = block_index (so we can check later)
    for (size_t block = 0; block < total_blocks; ++block) {
        for (size_t i = 0; i < buffer_size * channels; ++i) {
            in[i] = (float)block;
        }
        ck_assert_int_eq(ring_buffer_write(rb, in), 0);
    }

    // Now read the last 1:30 minutes, starting from 1:30 min ago up to most recent - 10s
    double read_start_sec = 90.0; // 1:30 min ago
    double read_duration_sec = (90.0 * sample_rate - 10.0 * sample_rate) / sample_rate; // up to most recent - 10s
    size_t total_frames_to_read = (size_t)(read_duration_sec * sample_rate);
    float *out = malloc(total_frames_to_read * channels * sizeof(float));
    ck_assert_ptr_ne(out, NULL);

    // Retrieve the full requested data in one call
    ck_assert_int_eq(ring_buffer_read(rb, out, read_start_sec, read_duration_sec), 0);
    // Print the first 256 samples for debugging
    for (size_t i = 0; i < 256 && i < total_frames_to_read; ++i) {
        printf("out[%zu] = %f\n", i, out[i]);
    }

    size_t total_blocks_to_read = total_frames_to_read / buffer_size;
    double block_duration = (double)buffer_size / sample_rate;
    size_t first_block_index = total_blocks - (size_t)(read_start_sec * sample_rate / buffer_size);
    for (size_t check = 0; check < 3; ++check) {
        size_t block;
        if (check == 0) block = 0; // beginning
        else if (check == 1) block = total_blocks_to_read / 2; // middle
        else block = total_blocks_to_read - 1; // end

        double seconds_ago = read_start_sec - block * block_duration;
        size_t expected_block = total_blocks - 1 - (size_t)(seconds_ago * sample_rate / buffer_size + 0.5);
        for (size_t i = 0; i < buffer_size * channels; ++i) {
            ck_assert_float_eq_tol(out[block * buffer_size * channels + i], (float)expected_block, 0.0001);
        }
    }

    free(in);
    free(out);
    ring_buffer_free(rb);
}
END_TEST

Suite *ring_buffer_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("RingBuffer");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_ring_buffer_init_params);
    tcase_add_test(tc_core, test_ring_buffer_write_read_basic);
    tcase_add_test(tc_core, test_ring_buffer_wrap_around);
    tcase_add_test(tc_core, test_ring_buffer_wrap_around_increasing);
    tcase_add_test(tc_core, test_ring_buffer_not_filled);
    tcase_add_test(tc_core, test_ring_buffer_out_of_bounds);
    tcase_add_test(tc_core, test_ring_buffer_two_channels);
    //tcase_add_test(tc_core, test_ring_buffer_real_life);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = ring_buffer_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}

