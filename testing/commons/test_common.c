#include <stdio.h>
#include "test_common.h"

#define main app_main
#include "../../main.c"
#undef main

/* QEMU semihosting SYS_EXIT */
static inline void qemu_exit(int status)
{
    __asm volatile (
        "mov r0, #0x18\n"
        "mov r1, %0\n"
        "bkpt #0xAB\n"
        :
        : "r"(status)
        : "r0", "r1"
    );
}

int main(void)
{
    test_result_t r = run_test();

    //Test pass is 0, Test fail is 1
    qemu_exit(r);

    for (;;);
}
