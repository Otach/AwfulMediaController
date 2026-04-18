#include <check.h>
#include <stdlib.h>
#include "amc_queue.h"

START_TEST(test_amcqueue_creation_and_free) {
    AMCQueue *q = amcqueue_init();
    ck_assert_int_eq(q->length, 0);
    ck_assert_ptr_null(q->head);
    ck_assert_ptr_null(q->tail);
    int r = amcqueue_free(q);
    ck_assert_int_eq(r, 0);
}
END_TEST

START_TEST(test_amcqueue_free) {
    AMCQueue *q = amcqueue_init();
    ck_assert_int_eq(q->length, 0);
    ck_assert_ptr_null(q->head);
    ck_assert_ptr_null(q->tail);
    const char *s = "This is a test string\0";
    amc_enqueue(q, (void *)s);
    ck_assert_int_eq(q->length, 1);
    int r = amcqueue_free(q);
    ck_assert_int_eq(r, -1);
    void *v = amc_dequeue(q);
    ck_assert_str_eq(s, v);
    r = amcqueue_free(q);
    ck_assert_int_eq(r, 0);
}
END_TEST

START_TEST(test_amcqueue_enqueue) {
    AMCQueue *q = amcqueue_init();
    const char *a = "Test String 0\0";
    const char *b = "Test String 1\0";

    ck_assert_int_eq(q->length, 0);
    ck_assert_ptr_null(q->head);
    ck_assert_ptr_null(q->tail);

    amc_enqueue(q, (void*)a);
    ck_assert_int_eq(q->length, 1);
    ck_assert_ptr_eq(q->head, q->tail);  // When there is only one node, head == tail

    amc_enqueue(q, (void*)b);
    ck_assert_int_eq(q->length, 2);
    ck_assert_ptr_ne(q->head, q->tail);
    ck_assert_ptr_eq(q->head->next, q->tail);
    ck_assert_ptr_null(q->tail->next);
}
END_TEST

START_TEST(test_amcqueue_dequeue) {
    const char *a = "Test String 0\0";
    const char *b = "Test String 1\0";
    AMCQueue *q = amcqueue_init();
    amc_enqueue(q, (void *)a);
    amc_enqueue(q, (void *)b);
    AMCNode *tail_node = q->tail;
    void *d = amc_dequeue(q);
    ck_assert_ptr_eq(d, a);
    ck_assert_ptr_eq(q->head, tail_node);
    ck_assert_ptr_eq(q->head, q->tail);
    ck_assert_int_eq(q->length, 1);

    d = amc_dequeue(q);
    ck_assert_ptr_eq(d, b);
    ck_assert_int_eq(q->length, 0);
    ck_assert_ptr_null(q->head);
    ck_assert_ptr_null(q->tail);

    int r = amcqueue_free(q);
    ck_assert_int_eq(r, 0);
}
END_TEST

START_TEST(test_amcqueue_peek) {
    const char *a = "Test String 0\0";
    const char *b = "Test String 1\0";
    AMCQueue *q = amcqueue_init();
    void *d = amcqueue_peek_nth(q, 1);
    ck_assert_ptr_null(d);
    amc_enqueue(q, (void *)a);

    d = amcqueue_peek_nth(q, 0);
    ck_assert_ptr_eq(d, a);
    ck_assert_int_eq(q->length, 1);

    d = amcqueue_peek_nth(q, 1);
    ck_assert_ptr_null(d);

    amc_enqueue(q, (void *)b);
    d = amcqueue_peek_nth(q, 1);
    ck_assert_ptr_eq(d, b);
    ck_assert_int_eq(q->length, 2);
}
END_TEST

START_TEST(test_amcqueue_remove) {
    const char *a = "Test String 0\0";
    const char *b = "Test String 1\0";
    const char *c = "Test String 2\0";
    AMCQueue *q = amcqueue_init();
    amc_enqueue(q, (void*)a);
    amc_enqueue(q, (void*)b);
    amc_enqueue(q, (void*)c);
    AMCNode *nc = q->tail;
    ck_assert_int_eq(q->length, 3);

    amcqueue_remove(q, (void *)b);
    ck_assert_ptr_eq(q->head->next, nc);
    ck_assert_int_eq(q->length, 2);

    // Remove head node
    amcqueue_remove(q, (void *)a);
    ck_assert_int_eq(q->length, 1);
    ck_assert_ptr_eq(q->head, nc);
    ck_assert_ptr_eq(q->head, q->tail);

    // Remove last element in list
    amcqueue_remove(q, (void *)c);
    ck_assert_int_eq(q->length, 0);
    ck_assert_ptr_null(q->head);
    ck_assert_ptr_null(q->tail);
}
END_TEST

Suite *amcqueue_suite(void) {
    Suite *s = suite_create("AMCQueue");

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_amcqueue_creation_and_free);
    tcase_add_test(tc_core, test_amcqueue_free);
    tcase_add_test(tc_core, test_amcqueue_enqueue);
    tcase_add_test(tc_core, test_amcqueue_dequeue);
    tcase_add_test(tc_core, test_amcqueue_peek);
    tcase_add_test(tc_core, test_amcqueue_remove);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void) {
    int number_failed;
    Suite *s = amcqueue_suite();
    SRunner *sr = srunner_create(s);

    // Run all tests and capture results
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr); // Cleanup
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
