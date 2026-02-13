#pragma once

#define TEST_PASS 0
#define TEST_FAIL 1

typedef int test_result_t;

test_result_t run_test(void);

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

#define ASSERT(cond)                \
    do {                                 \
        if (!(cond)) {                   \
            return TEST_FAIL;            \
        }                                \
    } while (0)



