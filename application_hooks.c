#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "timeline_scheduler.h"
#include "trace.h"   


/* Global variable to store the "High Score" of idle loops */
volatile uint32_t ulMaxIdleCountObserved = 0; 
#define SUBFRAME_DURATION_MS 100

void vApplicationIdleHook(void)
{
    static uint32_t ulIdleLoopCount = 0;
    
    /* 1. Increment the counter */
    ulIdleLoopCount++; 

    extern volatile uint32_t ulCurrentSubFrameIndex;
    extern volatile uint32_t ulGlobalTimeInFrame;
    static uint32_t ulLastSubframe = 0xFFFFFFFF;
    
    uint32_t ulSf = ulCurrentSubFrameIndex;
    
    /* Check for Subframe Boundary */
    if (ulSf != ulLastSubframe && ulLastSubframe != 0xFFFFFFFF)
    {
        /* AUTO-CALIBRATION: Update the Max if we found a new "laziest" frame */
        if (ulIdleLoopCount > ulMaxIdleCountObserved)
        {
            ulMaxIdleCountObserved = ulIdleLoopCount;
        }

        /* CALCULATION: (Current * 100) / Max */
        /* If Max is still 0 (first run), assume 0% to avoid divide-by-zero */
        uint32_t ulCalculatedMs = 0;
        
        if (ulMaxIdleCountObserved > 0)
        {
             // Use 64-bit math to prevent overflow: (Count * 100) / Max
            ulCalculatedMs = (uint32_t)( ( (uint64_t)ulIdleLoopCount * SUBFRAME_DURATION_MS ) / ulMaxIdleCountObserved );
        }

        /* Safety Clamp */
        if (ulCalculatedMs > SUBFRAME_DURATION_MS) ulCalculatedMs = SUBFRAME_DURATION_MS;

        /* Send the TIME */
        TracePushIdle((uint16_t)ulCalculatedMs, 
                      (uint16_t)ulGlobalTimeInFrame, 
                      (uint8_t)ulLastSubframe);
                      
        ulIdleLoopCount = 0; // Reset
    }
    
    ulLastSubframe = ulSf;
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
