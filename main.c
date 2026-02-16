#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "trace.h"
#include "timeline_scheduler.h"
#define WORK_LOAD_1MS 50000


volatile int sharedSensorData = 0;


// HRT tasks
// short and simple task
void vTask1(void *pvParams)
{
    (void)pvParams;
    UART_printf("HRT 1\n");
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS)*2; i++)
    {
        __asm volatile("nop");
    }
}

// heavy task
void vTask2(void *pvParams)
{
    (void)pvParams;
    UART_printf("HRT 2\n");
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS)*10; i++)
    {
        __asm volatile("nop");
    }
}

// intermediate level heavy task
void vTask3(void *pvParams)
{
    (void)pvParams;
    UART_printf("HRT 3\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS)*5; i++)
    {
        __asm volatile("nop");
    }

}


// SRT tasks
void vTaskSRT_A(void *pvParams) {
    (void)pvParams;
    UART_printf("SRT A\r\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS / 2); i++);
}

void vTaskSRT_B(void *pvParams) {
    (void)pvParams;
    UART_printf("SRT B\r\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS / 2); i++);
}

void vTaskProducer(void *pvParams) {
    (void)pvParams;
    // increment the value
    sharedSensorData++;
    UART_printf(" Producer wrote the data!\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2); i++);
}

void vTaskConsumer(void *pvParams) {
    (void)pvParams;
    // Reading the data (Polling)
    int read_data = sharedSensorData;

    UART_printf(" Consumer read the data!\n");
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2); i++);
}

/* Schedule table */
static TimelineTaskConfig_t my_schedule[] = {
    // Subframe 0: Data Exchange 
    {"Producer", vTaskProducer, HARD_RT, 2, 5, 0, 256, 0, NULL, TASK_NOT_STARTED, NULL},
    {"Consumer", vTaskConsumer, HARD_RT, 6, 9, 0, 256, 1, NULL, TASK_NOT_STARTED, NULL},

    // Subframe 2: A normal task to show life
    {"HRT_Mid", vTask1, HARD_RT, 22, 25, 0, 256, 2, NULL, TASK_NOT_STARTED, NULL},

    // SRT Tasks (Set times to 0 to be clean)
    {"SRT_A", vTaskSRT_A, SOFT_RT, 0, 0, 0, 256, 3, NULL, TASK_NOT_STARTED, NULL},
    {"SRT_B", vTaskSRT_B, SOFT_RT, 0, 0, 0, 256, 4, NULL, TASK_NOT_STARTED, NULL},
};

int main(void)
{
    UART_init();
    UART_printf("BOOT\r\n");

    /* init trace + start logging task */
    vTraceInit();
    vTraceRegisterNamesFromSchedule(my_schedule, 5);


    xTaskCreate(vLoggingTask,
                "logger",
                configMINIMAL_STACK_SIZE + 256,
                NULL,
                LOGGER_PRIORITY,
                NULL);

    const uint32_t subFrameCount = MAJOR_FRAME_DURATION_MS / MINOR_FRAME_DURATION_MS;
    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    // Pass the array, the number of tasks, subframe size, total subframes
    /*
     * Start the Timeline Scheduler.
     * Note: This function now handles validation internally using 'vApplicationScheduleErrorHook'.
     * We pass the schedule array, size, minor frame duration and total frames.
     */
    vStartTimelineScheduler(my_schedule, numTasks, MINOR_FRAME_DURATION_MS, subFrameCount);

    while (1)
    {
        // infinite loop (never reach here)
    }
}
