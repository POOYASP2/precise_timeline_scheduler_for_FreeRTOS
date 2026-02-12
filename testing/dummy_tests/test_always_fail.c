#include <stdio.h>

#include "testing/commons/test_common.h"

test_result_t run_test(void)
{
    ASSERT(1, 1);
    ASSERT(0, 1);
    return TEST_PASS;
}
