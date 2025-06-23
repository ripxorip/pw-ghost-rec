#include <check.h>
#include <stdlib.h>
#include "../src/audio-buffer.h"

START_TEST(test_audio_buffer_scaffold)
{
    audio_buffer_t ab;
    audio_buffer_init(&ab, 2, 16);
    audio_buffer_free(&ab);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("AudioBuffer");
    TCase *tc_core = tcase_create("Core");
    SRunner *sr = srunner_create(s);

    tcase_add_test(tc_core, test_audio_buffer_scaffold);
    suite_add_tcase(s, tc_core);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
