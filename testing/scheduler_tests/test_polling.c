#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"

//WARNING: This is a temporary generated test to check polling

// Match your style (workload loops with NOP)
#define WORK_LOAD_1MS 50000u

// Polling shared variable (same as your app)
volatile int sharedSensorData = 0;

// We capture what consumer read so we can assert deterministically.
static volatile int g_consumer_read = -1;

// Producer: increment sharedSensorData (same as app)
static void vTaskProducer(void *pvParams)
{
    (void)pvParams;

    sharedSensorData++;
    UART_printf("Producer wrote the data!\r\n");

    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2u); i++)
    {
        __asm volatile("nop");
    }
}

// Consumer: polling read (same as app), then assert correctness
static void vTaskConsumer(void *pvParams)
{
    (void)pvParams;

    int read_data = sharedSensorData;     // polling read
    g_consumer_read = read_data;

    UART_printf("Consumer read the data!\r\n");

    // Workload (same as app)
    for (volatile uint32_t i = 0; i < (WORK_LOAD_1MS * 2u); i++)
    {
        __asm volatile("nop");
    }

    // ---- TEST CHECKS HERE (no orchestrator) ----
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
    {"Producer", vTaskProducer, HARD_RT, 2, 5,  0, 256, 0, NULL, TASK_NOT_STARTED},
    {"Consumer", vTaskConsumer, HARD_RT, 6, 9,  0, 256, 1, NULL, TASK_NOT_STARTED},
};

test_result_t run_test(void)
{
    vTestPlatformBringUp(true);

    sharedSensorData = 0;
    g_consumer_read = -1;

    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    // For the test we use SF_MS=20, TOTAL_SF=3 like your other tests.
    // (If your project defines MINOR_FRAME_DURATION_MS, feel free to use that instead.)
    const uint32_t SF_MS = 20u;
    const uint32_t TOTAL_SF = 3u;

    ASSERT(xPreprocessSchedule(my_schedule, numTasks, SF_MS) == SCHED_VALID);
    ulSubFrameDuration = SF_MS;

    extern SchedError_t xValidateSchedule(const TimelineTaskConfig_t*, uint32_t, uint32_t, uint32_t);
    ASSERT(xValidateSchedule(my_schedule, numTasks, SF_MS, TOTAL_SF) == SCHED_VALID);

    vStartTimelineScheduler(my_schedule, numTasks, SF_MS, TOTAL_SF);

    //never reached
    return TEST_FAIL;
}
