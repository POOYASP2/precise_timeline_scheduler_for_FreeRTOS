#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "timeline_scheduler.h"


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
