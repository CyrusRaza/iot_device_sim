#include "unity.h"
#include "string_parser.h"

#include <threads.h>
#include <string.h>
#include <limits.h>

/* Internal functions from string_parser.c */
int string_to_int(const char *s, int *out);
int check_key(char *key);
void change_val(int val, int index);
void remove_spaces(char *restrict str_trimmed, const char *restrict str_untrimmed);
void parser(char *msg);

static int lock_initialized = 0;

static void reset_command_vals(void)
{
    mtx_lock(&command_vals_mutex);
    for (int i = 0; i < NUM_COMMANDS; i++) {
        command_vals[i] = 0;
    }
    mtx_unlock(&command_vals_mutex);
}

void setUp(void)
{
    if (!lock_initialized) {
        init_shared_lock();
        lock_initialized = 1;
    }
    reset_command_vals();
}

void tearDown(void)
{
}

/* ---------- command_list ---------- */

void test_command_list_has_expected_entries(void)
{
    TEST_ASSERT_EQUAL_STRING("temp_set", command_list[0]);
    TEST_ASSERT_EQUAL_STRING("pub_rate", command_list[1]);
    TEST_ASSERT_EQUAL_STRING("some1", command_list[2]);
    TEST_ASSERT_EQUAL_STRING("some2", command_list[3]);
    TEST_ASSERT_EQUAL_STRING("some3", command_list[4]);
}

/* ---------- string_to_int ---------- */

void test_string_to_int_parses_valid_positive_number(void)
{
    int out = 0;
    TEST_ASSERT_EQUAL_INT(0, string_to_int("123", &out));
    TEST_ASSERT_EQUAL_INT(123, out);
}

void test_string_to_int_parses_valid_negative_number(void)
{
    int out = 0;
    TEST_ASSERT_EQUAL_INT(0, string_to_int("-42", &out));
    TEST_ASSERT_EQUAL_INT(-42, out);
}

void test_string_to_int_rejects_null_string(void)
{
    int out = 0;
    TEST_ASSERT_EQUAL_INT(1, string_to_int(NULL, &out));
}

void test_string_to_int_rejects_null_output(void)
{
    TEST_ASSERT_EQUAL_INT(1, string_to_int("123", NULL));
}

void test_string_to_int_rejects_empty_string(void)
{
    int out = 0;
    TEST_ASSERT_EQUAL_INT(1, string_to_int("", &out));
}

void test_string_to_int_rejects_non_numeric_string(void)
{
    int out = 0;
    TEST_ASSERT_EQUAL_INT(1, string_to_int("abc", &out));
}

void test_string_to_int_rejects_trailing_characters(void)
{
    int out = 0;
    TEST_ASSERT_EQUAL_INT(1, string_to_int("123abc", &out));
}

void test_string_to_int_rejects_overflow(void)
{
    int out = 0;
    TEST_ASSERT_EQUAL_INT(1, string_to_int("999999999999999999999999", &out));
}

/* ---------- check_key ---------- */

void test_check_key_finds_temp_set(void)
{
    char key[] = "temp_set";
    TEST_ASSERT_EQUAL_INT(0, check_key(key));
}

void test_check_key_finds_pub_rate(void)
{
    char key[] = "pub_rate";
    TEST_ASSERT_EQUAL_INT(1, check_key(key));
}

void test_check_key_returns_minus_one_for_unknown_key(void)
{
    char key[] = "unknown";
    TEST_ASSERT_EQUAL_INT(-1, check_key(key));
}

/* ---------- remove_spaces ---------- */

void test_remove_spaces_removes_all_spaces(void)
{
    char input[] = "{ pub_rate : 20 , temp_set : 100 }";
    char output[128];

    remove_spaces(output, input);

    TEST_ASSERT_EQUAL_STRING("{pub_rate:20,temp_set:100}", output);
}

void test_remove_spaces_leaves_clean_string_unchanged(void)
{
    char input[] = "{pub_rate:20,temp_set:100}";
    char output[128];

    remove_spaces(output, input);

    TEST_ASSERT_EQUAL_STRING("{pub_rate:20,temp_set:100}", output);
}

/* ---------- change_val ---------- */

void test_change_val_updates_correct_index(void)
{
    change_val(55, 1);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[0]);
    TEST_ASSERT_EQUAL_INT(55, command_vals[1]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[2]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[3]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[4]);
    mtx_unlock(&command_vals_mutex);
}

/* ---------- parser ---------- */

void test_parser_updates_single_command(void)
{
    char msg[] = "{pub_rate:20}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[0]);   /* temp_set */
    TEST_ASSERT_EQUAL_INT(20, command_vals[1]);   /* pub_rate */
    TEST_ASSERT_EQUAL_INT(0,  command_vals[2]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[3]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[4]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_updates_multiple_commands(void)
{
    char msg[] = "{pub_rate:20, temp_set:100}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(100, command_vals[0]);  /* temp_set */
    TEST_ASSERT_EQUAL_INT(20,  command_vals[1]);  /* pub_rate */
    TEST_ASSERT_EQUAL_INT(0,   command_vals[2]);
    TEST_ASSERT_EQUAL_INT(0,   command_vals[3]);
    TEST_ASSERT_EQUAL_INT(0,   command_vals[4]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_handles_spaces_correctly(void)
{
    char msg[] = "{ temp_set : 77 , pub_rate : 33 }";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(77, command_vals[0]);
    TEST_ASSERT_EQUAL_INT(33, command_vals[1]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_ignores_unknown_key(void)
{
    char msg[] = "{unknown:55}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0, command_vals[0]);
    TEST_ASSERT_EQUAL_INT(0, command_vals[1]);
    TEST_ASSERT_EQUAL_INT(0, command_vals[2]);
    TEST_ASSERT_EQUAL_INT(0, command_vals[3]);
    TEST_ASSERT_EQUAL_INT(0, command_vals[4]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_updates_known_key_and_ignores_unknown_key(void)
{
    char msg[] = "{pub_rate:20, unknown:99}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[0]);
    TEST_ASSERT_EQUAL_INT(20, command_vals[1]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[2]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[3]);
    TEST_ASSERT_EQUAL_INT(0,  command_vals[4]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_rejects_non_numeric_value(void)
{
    char msg[] = "{pub_rate:abc}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0, command_vals[1]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_rejects_trailing_characters_in_value(void)
{
    char msg[] = "{pub_rate:20abc}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0, command_vals[1]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_rejects_overflow_value(void)
{
    char msg[] = "{pub_rate:999999999999999999999999}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0, command_vals[1]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_duplicate_key_last_value_wins(void)
{
    char msg[] = "{pub_rate:20, pub_rate:30}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(30, command_vals[1]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_missing_colon_is_ignored(void)
{
    char msg[] = "{pub_rate20}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    TEST_ASSERT_EQUAL_INT(0, command_vals[1]);
    mtx_unlock(&command_vals_mutex);
}

void test_parser_key_too_long_is_ignored(void)
{
    char msg[] = "{this_key_name_is_way_too_long:55}";

    parser(msg);

    mtx_lock(&command_vals_mutex);
    for (int i = 0; i < NUM_COMMANDS; i++) {
        TEST_ASSERT_EQUAL_INT(0, command_vals[i]);
    }
    mtx_unlock(&command_vals_mutex);
}

void test_print_commands_does_not_crash(void)
{
    print_commands();
    TEST_PASS();
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_command_list_has_expected_entries);

    RUN_TEST(test_string_to_int_parses_valid_positive_number);
    RUN_TEST(test_string_to_int_parses_valid_negative_number);
    RUN_TEST(test_string_to_int_rejects_null_string);
    RUN_TEST(test_string_to_int_rejects_null_output);
    RUN_TEST(test_string_to_int_rejects_empty_string);
    RUN_TEST(test_string_to_int_rejects_non_numeric_string);
    RUN_TEST(test_string_to_int_rejects_trailing_characters);
    RUN_TEST(test_string_to_int_rejects_overflow);

    RUN_TEST(test_check_key_finds_temp_set);
    RUN_TEST(test_check_key_finds_pub_rate);
    RUN_TEST(test_check_key_returns_minus_one_for_unknown_key);

    RUN_TEST(test_remove_spaces_removes_all_spaces);
    RUN_TEST(test_remove_spaces_leaves_clean_string_unchanged);

    RUN_TEST(test_change_val_updates_correct_index);

    RUN_TEST(test_parser_updates_single_command);
    RUN_TEST(test_parser_updates_multiple_commands);
    RUN_TEST(test_parser_handles_spaces_correctly);
    RUN_TEST(test_parser_ignores_unknown_key);
    RUN_TEST(test_parser_updates_known_key_and_ignores_unknown_key);
    RUN_TEST(test_parser_rejects_non_numeric_value);
    RUN_TEST(test_parser_rejects_trailing_characters_in_value);
    RUN_TEST(test_parser_rejects_overflow_value);
    RUN_TEST(test_parser_duplicate_key_last_value_wins);
    RUN_TEST(test_parser_missing_colon_is_ignored);
    RUN_TEST(test_parser_key_too_long_is_ignored);

    RUN_TEST(test_print_commands_does_not_crash);

    return UNITY_END();
}