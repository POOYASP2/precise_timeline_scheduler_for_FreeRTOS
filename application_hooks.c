#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "timeline_scheduler.h"
#include "trace.h"   // <-- add this

/* Log idle time as "ticks spent idle" per subframe. */
void vApplicationIdleHook(void)
{
    static TickType_t xLastTick = 0;
    static uint16_t   usIdleTicksThisSubframe = 0;
    static uint32_t   ulLastSubframe = 0xFFFFFFFFu;

    /* These are defined in FreeRTOS-Kernel/tasks.c */
    extern volatile uint32_t ulCurrentSubFrameIndex;
    extern volatile uint32_t ulGlobalTimeInFrame;

    /* Count idle ticks (only when tick changes to avoid overcounting). */
    TickType_t xNow = xTaskGetTickCount();
    if (xLastTick == 0) {
        xLastTick = xNow;
    } else if (xNow != xLastTick) {
        TickType_t xDiff = xNow - xLastTick; /* unsigned handles wrap */
        if (xDiff > 0xFFFFu) xDiff = 0xFFFFu;
        usIdleTicksThisSubframe = (uint16_t)(usIdleTicksThisSubframe + (uint16_t)xDiff);
        xLastTick = xNow;
    }

    /* Log once when the subframe changes. */
    uint32_t ulSf = ulCurrentSubFrameIndex;

    if (ulLastSubframe == 0xFFFFFFFFu) {
        ulLastSubframe = ulSf;
    }

    if (ulSf != ulLastSubframe) {
        if (usIdleTicksThisSubframe != 0) {
            /* frame_ms: current time in major frame (ms in your design),
               subframe: the one we just finished */
            TracePushIdle(usIdleTicksThisSubframe,
                          (uint16_t)ulGlobalTimeInFrame,
                          (uint8_t)ulLastSubframe);
        }

        usIdleTicksThisSubframe = 0;
        ulLastSubframe = ulSf;
    }
}

/*
 * ERROR HOOK
 * Defined by the application to handle critical scheduler errors.
 * This function is called by 'vStartTimelineScheduler' if validation fails.
 *
*/
void vApplicationScheduleErrorHook(SchedError_t xError)
{
    UART_printf("\n!WARNING!\nScheduler Error detected!!\n");
    UART_printf("\n-The system stopped-\n");

    switch (xError)
    {
        case ERR_OVERLAP:
            UART_printf("Reason: HRT Task Overlap Detected!\r\n");
            break;
        case ERR_OUT_OF_BOUNDS:
            UART_printf("Reason: Task Duration Exceeds Subframe Limit!\r\n");
            break;
        case ERR_INVALID_SF:
            UART_printf("Reason: Invalid Subframe ID configured!\r\n");
            break;
        case ERR_INVALID_TIME:
            UART_printf("Reason: Task End-Time is before Start-Time!\r\n");
            break;
        case ERR_PREPROCESS_FAIL:
            UART_printf("Reason: Preprocessing Failed! (Check Task Boundaries or Frame logic)\r\n"); 
            break;
        default:
            UART_printf("Reason: Unknown Error.\r\n");
            break;
    }

    taskDISABLE_INTERRUPTS();
    for (;;) {}
}
