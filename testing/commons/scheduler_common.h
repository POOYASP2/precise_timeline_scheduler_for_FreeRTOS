#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "drivers/uart.h"
#include "utils/trace.h"
#include "testing/commons/test_common.h"
#include "timeline_scheduler.h"

// These are provided by FreeRTOS-Kernel/tasks.c
extern volatile uint32_t ulCurrentSubFrameIndex;
extern volatile uint32_t ulGlobalTimeInFrame;
extern volatile uint32_t ulSubFrameDuration;

// Bring up UART + trace + (optional) logging task
static inline void vTestPlatformBringUp(bool startLogger)
{
    UART_init();
    UART_printf("TEST BOOT\r\n");

    vTraceInit();

    if (startLogger)
    {
        (void)xTaskCreate(vLoggingTask,
                          "logger",
                          configMINIMAL_STACK_SIZE + 256,
                          NULL,
                          tskIDLE_PRIORITY + 3,
                          NULL);
    }
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ASSERT_RTOS(cond)                                   \
    do {                                                    \
        if (!(cond)) {                                      \
            UART_printf("ASSERT FAIL: " __FILE__ ":"        \
                        TOSTRING(__LINE__) "\r\n");         \
            qemu_exit(TEST_FAIL);                            \
            for (;;) {}                                     \
        }                                                   \
    } while (0)


