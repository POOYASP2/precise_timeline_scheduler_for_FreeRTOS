#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "trace.h"
#include "timeline_scheduler.h"


SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule,
                               uint32_t uxTaskCount,
                               uint32_t ulSubFrameDuration,
                               uint32_t ulTotalSubFrames);

// HRT tasks
void vTask1(void *pvParams)
{
    (void)pvParams;
    for (volatile uint32_t i = 0; i < 50000; i++)
    {
        __asm volatile("nop");
    }
}

void vTask3(void *pvParams)
{
    (void)pvParams;
}

void vTask2(void *pvParams)
{
    (void)pvParams;
    for (volatile uint32_t i = 0; i < 400; i++)
    {
        __asm volatile("nop");
    }
}


// SRT tasks
void vTaskSRT_A(void *pvParams) {
    (void)pvParams;
    UART_printf("SRT A\r\n"); 
}

void vTaskSRT_B(void *pvParams) {
    (void)pvParams;
    UART_printf("SRT B\r\n");
}



/* Schedule table */
static TimelineTaskConfig_t my_schedule[] = {
    {"TASK 1", vTask1, HARD_RT, 10,   11,  0, 256, 0, NULL, TASK_NOT_STARTED, NULL},
    {"TASK 2", vTask2, HARD_RT, 200, 500, 0, 256, 1, NULL, TASK_NOT_STARTED, NULL},
    {"TASK 3", vTask2, HARD_RT, 550, 600, 0, 256, 2, NULL, TASK_NOT_STARTED, NULL},

    {"SRT A", vTaskSRT_A, SOFT_RT, 0, 0, 0, 256, 2, NULL, TASK_NOT_STARTED, NULL},
    {"SRT B", vTaskSRT_B, SOFT_RT, 0, 0, 0, 256, 3, NULL, TASK_NOT_STARTED, NULL},
};

int main(void)
{
    UART_init();
    UART_printf("BOOT\r\n");

    /* init trace + start logging task */
    vTraceInit();

    xTaskCreate(vLoggingTask,
                "logger",
                configMINIMAL_STACK_SIZE + 256,
                NULL,
                LOGGER_PRIORITY,
                NULL);

    const uint32_t subFrameCount = MAJOR_FRAME_DURATION_MS / MINOR_FRAME_DURATION_MS;
    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    if (xPreprocessSchedule(my_schedule, numTasks, MINOR_FRAME_DURATION_MS) == SCHED_VALID)
    {
        if (xValidateSchedule(my_schedule, numTasks, MINOR_FRAME_DURATION_MS, subFrameCount) == SCHED_VALID)
        {
            // Pass the array, the number of tasks, subframe size, total subframes
            vStartTimelineScheduler(my_schedule, numTasks, MINOR_FRAME_DURATION_MS, subFrameCount);
        }
        else
        {
            UART_printf("Schedule invalid\r\n");
            for (;;)
            {
            }
        }
    }
    else
    {
        UART_printf("Preprocess failed\r\n");
        for (;;)
        {
        }
    }

    while (1)
    {
        // infinite loop (never reach here)
    }
}

SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule,
                               uint32_t uxTaskCount,
                               uint32_t ulSubFrameDuration,
                               uint32_t ulTotalSubFrames)
{
    for (uint32_t i = 0; i < uxTaskCount; i++)
    {

        if (pxSchedule[i].type == SOFT_RT)
        {
            continue; // Skip to next task, SRTs don't need time checks
        }

        if (pxSchedule[i].ulEnd_time_ms <= pxSchedule[i].ulStart_time_ms)
        {
            return ERR_INVALID_TIME;
        }

        if (pxSchedule[i].ulEnd_time_ms > ulSubFrameDuration ||
            pxSchedule[i].ulStart_time_ms > ulSubFrameDuration)
        {
            return ERR_OUT_OF_BOUNDS;
        }

        if (pxSchedule[i].ulSubframe_id >= ulTotalSubFrames)
        {
            return ERR_INVALID_SF;
        }

        if (pxSchedule[i].type == HARD_RT)
        {
            for (uint32_t j = i + 1; j < uxTaskCount; j++)
            {
                if (pxSchedule[j].type == HARD_RT &&
                    pxSchedule[i].ulSubframe_id == pxSchedule[j].ulSubframe_id)
                {
                    if ((pxSchedule[i].ulStart_time_ms < pxSchedule[j].ulEnd_time_ms) &&
                        (pxSchedule[j].ulStart_time_ms < pxSchedule[i].ulEnd_time_ms))
                    {
                        return ERR_OVERLAP;
                    }
                }
            }
        }
    }

    return SCHED_VALID;
}
