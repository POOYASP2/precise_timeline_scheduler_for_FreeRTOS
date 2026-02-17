#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "trace.h"
#include "timeline_scheduler.h"

#define WORK_LOAD_1MS 50000

volatile int sharedSensorData = 0;

/* --------------------------------------------------------------------------
 * Generated schedule (tools/schedule.json -> generated/schedule_config.c)
 * -------------------------------------------------------------------------- */
extern TimelineTaskConfig_t my_schedule[];
extern const uint32_t my_schedule_count;

/* Generated timing params */
extern const uint32_t g_major_frame_ms;
extern const uint32_t g_minor_frame_ms;
extern const uint32_t g_subframe_count;

// HRT tasks
// short and simple task
void vTask1(void *pvParams)
{
    (void)pvParams;
    UART_printf("HRT 1\n");
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS) * 2; i++)
    {
        __asm volatile("nop");
    }
}

// heavy task
void vTask2(void *pvParams)
{
    (void)pvParams;
    UART_printf("HRT 2\n");
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS) * 10; i++)
    {
        __asm volatile("nop");
    }
}

// intermediate level heavy task
void vTask3(void *pvParams)
{
    (void)pvParams;
    UART_printf("HRT  3\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS) * 5; i++)
    {
        __asm volatile("nop");
    }
}

// SRT tasks
void vTaskSRT_A(void *pvParams)
{
    (void)pvParams;
    UART_printf("SRT A\r\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS / 2); i++)
        ;
}

void vTaskSRT_B(void *pvParams)
{
    (void)pvParams;
    UART_printf("SRT B\r\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS / 2); i++)
        ;
}

void vTaskProducer(void *pvParams)
{
    (void)pvParams;
    // increment the value
    sharedSensorData++;
    UART_printf(" Producer wrote the data!\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2); i++)
        ;
}

void vTaskConsumer(void *pvParams)
{
    (void)pvParams;
    // Reading the data (Polling)
    int read_data = sharedSensorData;
    (void)read_data;

    UART_printf(" Consumer read the data!\n");
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2); i++)
        ;
}

int main(void)
{
    UART_init();
    UART_printf("BOOT\r\n");

    /* init trace + start logging task */
    vTraceInit();

    /* Register task names based on generated schedule */
    vTraceRegisterNamesFromSchedule(my_schedule, my_schedule_count);

    xTaskCreate(vLoggingTask,
                "logger",
                configMINIMAL_STACK_SIZE + 256,
                NULL,
                LOGGER_PRIORITY,
                NULL);

    /* Use generated frame parameters (from schedule.json) */
    const uint32_t subFrameCount = g_subframe_count; /* == g_major_frame_ms / g_minor_frame_ms */
    const uint32_t numTasks = my_schedule_count;

    /*
     * Start the Timeline Scheduler.
     * Note: This function now handles validation internally using 'vApplicationScheduleErrorHook'.
     * We pass the schedule array, size, minor frame duration and total frames.
     */
    vStartTimelineScheduler(my_schedule, numTasks, g_minor_frame_ms, subFrameCount);

    while (1)
    {
        // infinite loop (never reach here)
    }
}
