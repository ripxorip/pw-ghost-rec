#include <check.h>
#include "../src/ring-buffer.h"

START_TEST(test_ring_buffer_init)
{
    // Dummy test: just call the init function
    ring_buffer_init();
    ck_assert_int_eq(1, 1); // Dummy assertion
}
END_TEST

Suite *ring_buffer_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("RingBuffer");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_ring_buffer_init);
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

