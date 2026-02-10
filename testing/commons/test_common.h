#pragma once

#define TEST_PASS 0
#define TEST_FAIL 1

typedef int test_result_t;

test_result_t run_test(void);

#define TEST_ASSERT(cond)                \
    do {                                 \
        if (!(cond)) {                   \
            return TEST_FAIL;            \
        }                                \
    } while (0)


