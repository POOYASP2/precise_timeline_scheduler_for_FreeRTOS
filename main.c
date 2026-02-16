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
    // Subframe 0: Data Exchange 
    {"Producer", vTaskProducer, HARD_RT, 2, 5, 0, 256, 0, NULL, TASK_NOT_STARTED, NULL},
    {"Consumer", vTaskConsumer, HARD_RT, 6, 9, 0, 256, 1, NULL, TASK_NOT_STARTED, NULL},

    // Subframe 2: A normal task to show life
    {"HRT_Mid", vTask1, HARD_RT, 22, 25, 0, 256, 2, NULL, TASK_NOT_STARTED, NULL},

    // SRT Tasks (Set times to 0 to be clean)
    {"SRT_A", vTaskSRT_A, SOFT_RT, 0, 0, 0, 256, 3, NULL, TASK_NOT_STARTED, NULL},
    {"SRT_B", vTaskSRT_B, SOFT_RT, 0, 0, 0, 256, 4, NULL, TASK_NOT_STARTED, NULL},
};

static void vTraceRegisterNamesFromSchedule(void)
{
    uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    for (uint32_t i = 0; i < numTasks; i++)
    {
        /* These field names match your timeline_scheduler.c usage: pxScheduleTable[i].task_name and .taskId */
        TraceRegisterTaskName((uint8_t)my_schedule[i].taskId, my_schedule[i].task_name);
    }
}

int main(void)
{
    UART_init();
    UART_printf("BOOT\r\n");

    /* init trace + start logging task */
    vTraceInit();

    vTraceRegisterNamesFromSchedule();


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
