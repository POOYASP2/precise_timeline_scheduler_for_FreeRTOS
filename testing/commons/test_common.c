#include <stdio.h>
#include "test_common.h"

#define main app_main
#include "../../main.c"
#undef main


int main(void)
{
    test_result_t r = run_test();

    //Test pass is 0, Test fail is 1
    qemu_exit(r);

    for (;;);
}
