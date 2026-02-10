#include <stdio.h>
#include "test_common.h"

#define main app_main
#include "../../main.c"
#undef main

/* QEMU semihosting SYS_EXIT_EXT */
static inline void qemu_exit(int status)
{
    volatile int params[2] = { 0x20026, status }; // ADP_Stopped_ApplicationExit
    __asm volatile (
        "mov r0, #0x20\n"     // SYS_EXIT_EXT
        "mov r1, %0\n"
        "bkpt #0xAB\n"
        :
        : "r"(params)
        : "r0", "r1", "memory"
    );
}


int main(void)
{
    test_result_t r = run_test();

    //Test pass is 0, Test fail is 1
    qemu_exit(r);

    for (;;);
}
