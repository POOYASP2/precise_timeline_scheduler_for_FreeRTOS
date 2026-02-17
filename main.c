#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "trace.h"
#include "timeline_scheduler.h"

extern TimelineTaskConfig_t my_schedule[];
extern const uint32_t my_schedule_count;

/* timing params */
extern const uint32_t g_major_frame_ms;
extern const uint32_t g_minor_frame_ms;
extern const uint32_t g_subframe_count;

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

    // Generated frame parameters
    const uint32_t subFrameCount = g_subframe_count; /* == g_major_frame_ms / g_minor_frame_ms */
    const uint32_t numTasks = my_schedule_count;

    // Start the timeline scheduler
    vStartTimelineScheduler(my_schedule, numTasks, g_minor_frame_ms, subFrameCount);

    while (1)
    {
        // we never reach here
    }
}
