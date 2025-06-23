#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/ring-buffer.h"

START_TEST(test_ringbuffer_init_and_free)
{
    ringbuffer_float_t rb;
    uint32_t size = 10;
    ringbuffer_float_init(&rb, size);
    ck_assert_ptr_nonnull(rb.buffer);
    ck_assert_int_eq(rb.size, size);
    ck_assert_int_eq(rb.start, 0);
    ck_assert_int_eq(rb.end, size - 1);
    ringbuffer_float_free(&rb);
    ck_assert_ptr_null(rb.buffer);
}
END_TEST

START_TEST(test_ringbuffer_write_read)
{
    ringbuffer_float_t rb;
    uint32_t size = 5;
    ringbuffer_float_init(&rb, size);

    // Write values 1.0, 2.0, ..., 5.0
    for (uint32_t i = 0; i < size; ++i) {
        float v = (float)(i + 1);
        ringbuffer_float_write(&rb, &v);
    }

    // Read back values with offset 0 (last written) to size-1 (first written)
    for (uint32_t i = 0; i < size; ++i) {
        float v = 0.0f;
        ringbuffer_float_get_value(&rb, &v, i);
        ck_assert_float_eq_tol(v, (float)(size - i), 1e-6);
    }

    ringbuffer_float_free(&rb);
}
END_TEST

START_TEST(test_ringbuffer_wrap_around)
{
    ringbuffer_float_t rb;
    uint32_t size = 3;
    ringbuffer_float_init(&rb, size);

    // Fill buffer
    for (uint32_t i = 0; i < size; ++i) {
        float v = (float)(i + 1); // 1.0, 2.0, 3.0
        ringbuffer_float_write(&rb, &v);
    }
    // Write one more to wrap around
    float v = 4.0f;
    ringbuffer_float_write(&rb, &v);

    // Now buffer should contain 4.0, 2.0, 3.0 (4.0 overwrote 1.0)
    float out;
    ringbuffer_float_get_value(&rb, &out, 0); // last written
    ck_assert_float_eq_tol(out, 4.0f, 1e-6);
    ringbuffer_float_get_value(&rb, &out, 1);
    ck_assert_float_eq_tol(out, 3.0f, 1e-6);
    ringbuffer_float_get_value(&rb, &out, 2);
    ck_assert_float_eq_tol(out, 2.0f, 1e-6);

    ringbuffer_float_free(&rb);
}
END_TEST

START_TEST(test_ringbuffer_wrap_around_with_offset)
{
    ringbuffer_float_t rb;
    uint32_t size = 3;
    ringbuffer_float_init(&rb, size);

    // Write 1, 2, 3
    for (uint32_t i = 0; i < size; ++i) {
        float v = (float)(i + 1);
        ringbuffer_float_write(&rb, &v);
    }
    // Write 4, 5 (wraps around twice)
    float v4 = 4.0f, v5 = 5.0f;
    ringbuffer_float_write(&rb, &v4);
    ringbuffer_float_write(&rb, &v5);

    // Buffer now: 4.0, 5.0, 3.0 (5.0 overwrote 2.0, 4.0 overwrote 1.0)
    // Offsets: 0=5.0, 1=4.0, 2=3.0
    float out;
    ringbuffer_float_get_value(&rb, &out, 0);
    ck_assert_float_eq_tol(out, 5.0f, 1e-6);
    ringbuffer_float_get_value(&rb, &out, 1);
    ck_assert_float_eq_tol(out, 4.0f, 1e-6);
    ringbuffer_float_get_value(&rb, &out, 2);
    ck_assert_float_eq_tol(out, 3.0f, 1e-6);

    ringbuffer_float_free(&rb);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("RingBuffer");
    TCase *tc_core = tcase_create("Core");
    SRunner *sr = srunner_create(s);

    tcase_add_test(tc_core, test_ringbuffer_init_and_free);
    tcase_add_test(tc_core, test_ringbuffer_write_read);
    tcase_add_test(tc_core, test_ringbuffer_wrap_around);
    tcase_add_test(tc_core, test_ringbuffer_wrap_around_with_offset);
    suite_add_tcase(s, tc_core);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

