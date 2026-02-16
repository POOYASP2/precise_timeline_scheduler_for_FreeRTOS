#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"
#include <string.h>

#define WORK_LOAD_1MS 50000u

// Polling shared variable
volatile int shared_data = 0;

// We capture what consumer read so we can assert deterministically.
static volatile int g_consumer_read = -1;

// Producer: increment shared_data
static void vTaskProducer(void *pvParams)
{
    (void)pvParams;

    shared_data++;
    UART_printf("Producer wrote the data!\r\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2u); i++)
    {
        __asm volatile("nop");
    }
}

// Consumer: polling read, then assert correctness
static void vTaskConsumer(void *pvParams)
{
    (void)pvParams;

    int read_data = shared_data;     // polling read
    g_consumer_read = read_data;

    UART_printf("Consumer read the data!\r\n");

    // Workload
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2u); i++)
    {
        __asm volatile("nop");
    }

    // Consumer is scheduled after Producer, so it must observe the increment.
    if (read_data == 1)
    {
        UART_printf("POLLING TEST PASS\r\n");
        qemu_exit(TEST_PASS);
    }
    else
    {
        UART_printf("POLLING TEST FAIL\r\n");
        qemu_exit(TEST_FAIL);
    }

    for (;;) {}
}

/* Schedule table
   Mirrors your example ordering and tight timing:
   - Producer runs first
   - Consumer runs after
   Everything is within the same subframe (subframe 0) for simplicity.
*/
static TimelineTaskConfig_t my_schedule[] = {
    {"Producer", vTaskProducer, HARD_RT, 2, 28,  0, 256, 0, NULL, TASK_NOT_STARTED},
    {"Consumer", vTaskConsumer, HARD_RT, 30, 58,  0, 256, 1, NULL, TASK_NOT_STARTED},
};

test_result_t run_test(void)
{

    shared_data = 0;
    g_consumer_read = -1;

    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    const uint32_t SF_MS = 60u;
    const uint32_t TOTAL_SF = 3u;

    TimelineTaskConfig_t sched_copy[2];
    memcpy(sched_copy, my_schedule, sizeof(my_schedule));

    ASSERT(xPreprocessSchedule(sched_copy, 2, SF_MS) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched_copy, 2, SF_MS, TOTAL_SF) == SCHED_VALID);

    vTestPlatformBringUp(true, my_schedule, 2);

    vStartTimelineScheduler(my_schedule, 2, SF_MS, TOTAL_SF);

    //never reached
    return TEST_FAIL;
}
