/*
 * fn_test.h — tiny single-header C99 assert harness for T1 host unit tests.
 *
 * The repo-wide convention: a T1 test is a plain C program that runs natively
 * (native `cc`, no AmigaOS, no backend). It increments `fn_failures` for every
 * failed check and returns that count from main() — non-zero == test failed.
 * This formalizes the printf/failure-counter pattern already used by the T2
 * smoke test in ../compat_test.c.
 *
 * Usage:
 *     #define FN_TEST_MAIN          // in exactly ONE .c file, before including
 *     #include "fn_test.h"
 *     ...
 *     int main(void) {
 *         CHECK(some_condition);
 *         CHECK_EQ(actual, expected);
 *         return fn_test_report("test_name");
 *     }
 *
 * See docs/testing.md for the full testing practice.
 */
#ifndef FN_TEST_H
#define FN_TEST_H

#include <stdio.h>

extern int fn_failures;
extern int fn_checks;

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++fn_checks;                                                        \
        if (!(cond)) {                                                      \
            ++fn_failures;                                                  \
            printf("FAIL %s:%d: CHECK(%s)\n", __FILE__, __LINE__, #cond);   \
        }                                                                  \
    } while (0)

#define CHECK_EQ(actual, expected)                                         \
    do {                                                                   \
        ++fn_checks;                                                        \
        long _a = (long)(actual);                                          \
        long _e = (long)(expected);                                        \
        if (_a != _e) {                                                     \
            ++fn_failures;                                                  \
            printf("FAIL %s:%d: CHECK_EQ(%s, %s) -> got %ld, want %ld\n",   \
                   __FILE__, __LINE__, #actual, #expected, _a, _e);         \
        }                                                                  \
    } while (0)

/* Print a one-line summary and return the failure count (main's exit code). */
static int fn_test_report(const char *name)
{
    if (fn_failures == 0)
        printf("PASS %s (%d checks)\n", name, fn_checks);
    else
        printf("%d/%d CHECK(S) FAILED in %s\n", fn_failures, fn_checks, name);
    return fn_failures;
}

#ifdef FN_TEST_MAIN
int fn_failures = 0;
int fn_checks   = 0;
#endif

#endif /* FN_TEST_H */
