#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "trace.h"
#include "timeline_scheduler.h"

#define MAJOR_FRAME_DURATION_MS 5000
#define MINOR_FRAME_DURATION_MS 1000

SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule,
                               uint32_t uxTaskCount,
                               uint32_t ulSubFrameDuration,
                               uint32_t ulTotalSubFrames);

/* Dummy task bodies: do some work then return */
static void vTask1(void *pvParams)
{
    (void)pvParams;
    for (volatile uint32_t i = 0; i < 350; i++)
    {
        __asm volatile("nop");
    }
}

static void vTask2(void *pvParams)
{
    (void)pvParams;
    for (volatile uint32_t i = 0; i < 400; i++)
    {
        __asm volatile("nop");
    }
}

/* Schedule table */
static TimelineTaskConfig_t my_schedule[] = {
    {"TASK 1", vTask1, HARD_RT, 0, 50, 0, 256, 0, NULL, TASK_NOT_STARTED},
    {"TASK 2", vTask2, HARD_RT, 200, 500, 0, 256, 1, NULL, TASK_NOT_STARTED},
    {"TASK 3", vTask2, HARD_RT, 550, 600, 0, 256, 2, NULL, TASK_NOT_STARTED},
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
                tskIDLE_PRIORITY + 3,
                NULL);

    const uint32_t subFrameCount = MAJOR_FRAME_DURATION_MS / MINOR_FRAME_DURATION_MS;
    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    SchedError_t ok = xValidateSchedule(my_schedule, numTasks,
                                        MINOR_FRAME_DURATION_MS,
                                        subFrameCount);

    if (ok != SCHED_VALID)
    {
        UART_printf("Schedule invalid\r\n");
        for (;;)
        {
        }
    }

    UART_printf("Starting timeline scheduler\r\n");
    vStartTimelineScheduler(my_schedule, numTasks, MINOR_FRAME_DURATION_MS, subFrameCount);

    UART_printf("Unexpected return\r\n");
    for (;;)
    {
    }
}

SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule,
                               uint32_t uxTaskCount,
                               uint32_t ulSubFrameDuration,
                               uint32_t ulTotalSubFrames)
{
    for (uint32_t i = 0; i < uxTaskCount; i++)
    {
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
