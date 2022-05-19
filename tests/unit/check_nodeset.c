#include <check.h>

#include <nodeset.h>

#include <limits.h>
#include <stdlib.h>

START_TEST(nodeset_empty_input)
{
    struct nodeset *n = nodeset_from_str("");

    ck_assert_ptr_null(n);
}
END_TEST

START_TEST(nodeset_empty)
{
    struct nodeset *n = nodeset_from_str("[]");

    ck_assert_ptr_nonnull(n);
    ck_assert_uint_eq(n->type, NODESET_EMPTY);

    nodeset_destroy(n);
}
END_TEST

void test_single_element(const char *input, unsigned int expected)
{
    struct nodeset *n = nodeset_from_str(input);

    ck_assert_ptr_nonnull(n);
    ck_assert_uint_eq(n->type, NODESET_VALUE);
    ck_assert_uint_eq(n->data.value, expected);

    nodeset_destroy(n);
}

START_TEST(nodeset_simple_elements)
{
    test_single_element("[1]", 1);
    test_single_element("[0]", 0);
    test_single_element("[10]", 10);
}
END_TEST

START_TEST(nodeset_overflow_uint)
{
    struct nodeset *n;
    char *input;
    int rc;

    rc = asprintf(&input, "[%lu]", (unsigned long)(UINT_MAX) + 1);
    ck_assert_int_ne(rc, -1);

    n = nodeset_from_str(input);
    free(input);

    ck_assert_ptr_null(n);
}
END_TEST

START_TEST(nodeset_overflow_ulong)
{
    struct nodeset *n;
    char *input;
    int rc;

    rc = asprintf(&input, "[%lu55]", ULONG_MAX);
    ck_assert_int_ne(rc, -1);

    n = nodeset_from_str(input);
    free(input);

    ck_assert_ptr_null(n);
}
END_TEST

void test_invalid_input(const char *input)
{
    struct nodeset *n = nodeset_from_str(input);

    ck_assert_ptr_null(n);
}

START_TEST(nodeset_invalid_inputs)
{
    test_invalid_input("[invalid]");
    test_invalid_input("[-10]");
    test_invalid_input("[-]");
    test_invalid_input("[,]");
}
END_TEST

START_TEST(nodeset_single_range)
{
    struct nodeset *n = nodeset_from_str("[2-5]");

    ck_assert_uint_eq(n->type, NODESET_RANGE);
    ck_assert_uint_eq(n->data.range.begin, 2);
    ck_assert_uint_eq(n->data.range.end, 5);
}
END_TEST

START_TEST(nodeset_two_elements)
{
    struct nodeset *n = nodeset_from_str("[1,2-5]");

    ck_assert_ptr_nonnull(n);
    ck_assert_ptr_nonnull(n->next);

    ck_assert_uint_eq(n->type, NODESET_VALUE);
    ck_assert_uint_eq(n->data.value, 1);

    ck_assert_uint_eq(n->next->type, NODESET_RANGE);
    ck_assert_uint_eq(n->next->data.range.begin, 2);
    ck_assert_uint_eq(n->next->data.range.end, 5);

    ck_assert_ptr_null(n->next->next);

    nodeset_destroy(n);
}
END_TEST

START_TEST(nodeset_complex_invalid_input)
{
    test_invalid_input("[14test7]");
    test_invalid_input("[14-2,-4]");
    test_invalid_input("[14-2,,]");
}
END_TEST

static Suite *
unit_suite(void)
{
    Suite *suite;
    TCase *tests;

    suite = suite_create("nodeset");
    tests = tcase_create("nodeset_from_str()");

    tcase_add_test(tests, nodeset_empty_input);
    tcase_add_test(tests, nodeset_empty);
    tcase_add_test(tests, nodeset_simple_elements);
    tcase_add_test(tests, nodeset_overflow_uint);
    tcase_add_test(tests, nodeset_overflow_ulong);
    tcase_add_test(tests, nodeset_invalid_inputs);
    tcase_add_test(tests, nodeset_single_range);
    tcase_add_test(tests, nodeset_two_elements);
    tcase_add_test(tests, nodeset_complex_invalid_input);

    suite_add_tcase(suite, tests);

    return suite;
}

int main()
{
    int number_failed;
    SRunner *runner;
    Suite *suite;

    suite = unit_suite();
    runner = srunner_create(suite);

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
