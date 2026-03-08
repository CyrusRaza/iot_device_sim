#include "unity.h"
#include "shared_mem.h"

#include <string.h>

static shared_mem *queue;

void setUp(void)
{
    queue = queue_create();
    TEST_ASSERT_NOT_NULL(queue);
}

void tearDown(void)
{
    if (queue != NULL) {
        queue_destroy(queue);
        queue = NULL;
    }
}

/* ---------- create / initial state ---------- */

void test_queue_create_initializes_empty_queue(void)
{
    TEST_ASSERT_TRUE(queue_is_empty(queue));
    TEST_ASSERT_FALSE(queue_is_full(queue));
    TEST_ASSERT_EQUAL_INT(0, queue_filled_spaces(queue));
    TEST_ASSERT_EQUAL_INT(BUFFER_SIZE, queue_free_spaces(queue));
}

/* ---------- push / pop ---------- */

void test_queue_push_single_message_succeeds(void)
{
    char msg[] = "{pub_rate:20}";

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg, (int)strlen(msg)));

    TEST_ASSERT_FALSE(queue_is_empty(queue));
    TEST_ASSERT_EQUAL_INT(1, queue_filled_spaces(queue));
}

void test_queue_push_then_pop_returns_same_message(void)
{
    char msg[] = "{pub_rate:20}";
    char out[128] = {0};

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg, (int)strlen(msg)));
    TEST_ASSERT_EQUAL_INT(0, queue_pop(queue, out));

    TEST_ASSERT_EQUAL_STRING("{pub_rate:20}", out);
    TEST_ASSERT_TRUE(queue_is_empty(queue));
    TEST_ASSERT_EQUAL_INT(0, queue_filled_spaces(queue));
}

void test_queue_preserves_fifo_order(void)
{
    char out[128] = {0};

    char msg1[] = "{pub_rate:20}";
    char msg2[] = "{temp_set:100}";
    char msg3[] = "{some1:7}";

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg1, (int)strlen(msg1)));
    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg2, (int)strlen(msg2)));
    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg3, (int)strlen(msg3)));

    TEST_ASSERT_EQUAL_INT(0, queue_pop(queue, out));
    TEST_ASSERT_EQUAL_STRING("{pub_rate:20}", out);

    TEST_ASSERT_EQUAL_INT(0, queue_pop(queue, out));
    TEST_ASSERT_EQUAL_STRING("{temp_set:100}", out);

    TEST_ASSERT_EQUAL_INT(0, queue_pop(queue, out));
    TEST_ASSERT_EQUAL_STRING("{some1:7}", out);

    TEST_ASSERT_TRUE(queue_is_empty(queue));
}

/* ---------- empty behavior ---------- */

void test_queue_pop_from_empty_returns_error(void)
{
    char out[128] = {0};

    TEST_ASSERT_EQUAL_INT(1, queue_pop(queue, out));
    TEST_ASSERT_TRUE(queue_is_empty(queue));
}

/* ---------- capacity behavior ---------- */

void test_queue_reports_non_empty_after_push(void)
{
    char msg[] = "{pub_rate:20}";

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg, (int)strlen(msg)));

    TEST_ASSERT_FALSE(queue_is_empty(queue));
}

void test_queue_filled_spaces_increases_with_pushes(void)
{
    char msg1[] = "{pub_rate:20}";
    char msg2[] = "{temp_set:100}";

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg1, (int)strlen(msg1)));
    TEST_ASSERT_EQUAL_INT(1, queue_filled_spaces(queue));

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg2, (int)strlen(msg2)));
    TEST_ASSERT_EQUAL_INT(2, queue_filled_spaces(queue));
}

void test_queue_free_spaces_decreases_with_pushes(void)
{
    char msg1[] = "{pub_rate:20}";
    char msg2[] = "{temp_set:100}";

    TEST_ASSERT_EQUAL_INT(BUFFER_SIZE, queue_free_spaces(queue));

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg1, (int)strlen(msg1)));
    TEST_ASSERT_EQUAL_INT(BUFFER_SIZE - 1, queue_free_spaces(queue));

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg2, (int)strlen(msg2)));
    TEST_ASSERT_EQUAL_INT(BUFFER_SIZE - 2, queue_free_spaces(queue));
}

/*
 * This test assumes your full condition allows BUFFER_SIZE items.
 * If you intentionally reserve one slot, change BUFFER_SIZE to BUFFER_SIZE - 1.
 */
void test_queue_becomes_full_at_capacity(void)
{
    char msg[] = "X";
    int i;

    for (i = 0; i < BUFFER_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg, (int)strlen(msg)));
    }

    TEST_ASSERT_TRUE(queue_is_full(queue));
    TEST_ASSERT_EQUAL_INT(BUFFER_SIZE, queue_filled_spaces(queue));
    TEST_ASSERT_EQUAL_INT(0, queue_free_spaces(queue));
}

void test_queue_push_when_full_returns_error(void)
{
    char msg[] = "X";
    int i;

    for (i = 0; i < BUFFER_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg, (int)strlen(msg)));
    }

    TEST_ASSERT_EQUAL_INT(1, queue_push(queue, msg, (int)strlen(msg)));
}

/* ---------- wraparound behavior ---------- */

void test_queue_wraparound_still_preserves_fifo_order(void)
{
    char out[128] = {0};
    char msg[] = "X";

    for (int i = 0; i < BUFFER_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT(0, queue_push(queue, msg, (int)strlen(msg)));
    }

    for (int i = 0; i < BUFFER_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT(0, queue_pop(queue, out));
        TEST_ASSERT_EQUAL_STRING("X", out);
    }

    TEST_ASSERT_TRUE(queue_is_empty(queue));

    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, "{pub_rate:20}", 13));
    TEST_ASSERT_EQUAL_INT(0, queue_push(queue, "{temp_set:100}", 14));

    TEST_ASSERT_EQUAL_INT(0, queue_pop(queue, out));
    TEST_ASSERT_EQUAL_STRING("{pub_rate:20}", out);

    TEST_ASSERT_EQUAL_INT(0, queue_pop(queue, out));
    TEST_ASSERT_EQUAL_STRING("{temp_set:100}", out);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_queue_create_initializes_empty_queue);

    RUN_TEST(test_queue_push_single_message_succeeds);
    RUN_TEST(test_queue_push_then_pop_returns_same_message);
    RUN_TEST(test_queue_preserves_fifo_order);

    RUN_TEST(test_queue_pop_from_empty_returns_error);

    RUN_TEST(test_queue_reports_non_empty_after_push);
    RUN_TEST(test_queue_filled_spaces_increases_with_pushes);
    RUN_TEST(test_queue_free_spaces_decreases_with_pushes);

    RUN_TEST(test_queue_becomes_full_at_capacity);
    RUN_TEST(test_queue_push_when_full_returns_error);

    RUN_TEST(test_queue_wraparound_still_preserves_fifo_order);

    return UNITY_END();
}