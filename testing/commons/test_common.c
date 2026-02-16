#include <stdio.h>
#include "test_common.h"

int main(void)
{
    test_result_t r = run_test();

    //Test pass is 0, Test fail is 1
    qemu_exit(r);

    for (;;);
}
