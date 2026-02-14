#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "trace.h"
#include "timeline_scheduler.h"
#define WORK_LOAD_1MS 50000



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

/*
 * ERROR HOOK
 * Defined by the application to handle critical scheduler errors.
 * This function is called by 'vStartTimelineScheduler' if validation fails.
 *
*/
void vApplicationScheduleErrorHook(SchedError_t xError)
{
	UART_printf("\n!WARNING!\nScheduler Error detected!!\n");
	//UART_printf("Error code: %d\n", xError);
	UART_printf("\n-The system stopped-\n");

	switch(xError)
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
	// here lock the system with an infinite loop
	taskDISABLE_INTERRUPTS();
    	for(;;);
}


/* Schedule table */
static TimelineTaskConfig_t my_schedule[] = {
    // HRT1: runs in the beginning between 2ms and 8ms; subframe 0
    {"HRT_Start", vTask1, HARD_RT, 2, 8, 0, 256, 0, NULL, TASK_NOT_STARTED, NULL},
    // HRT2: runs in the middle between 45ms and 48ms; subframe 4
    {"HRT_Mid",   vTask1, HARD_RT, 45, 48, 4, 256, 1, NULL, TASK_NOT_STARTED, NULL},
    // HRT3: runs in the end between 90ms and 95ms; subframe 9
    {"HRT_End",   vTask3, HARD_RT, 90, 95, 9, 256, 2, NULL, TASK_NOT_STARTED, NULL},

    // SRT Tasks, fills the gaps
    {"SRT_A",     vTaskSRT_A, SOFT_RT, 0, 0, 0, 256, 3, NULL, TASK_NOT_STARTED, NULL},
    {"SRT_B",     vTaskSRT_B, SOFT_RT, 0, 0, 0, 256, 4, NULL, TASK_NOT_STARTED, NULL},
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
