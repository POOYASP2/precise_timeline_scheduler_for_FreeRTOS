#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"
#include <string.h>

#define MAJOR_MS   500u
#define SF_MS      100u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

static volatile uint32_t g_started = 0;

static void vTinyWork(void *pv)
{
    (void)pv;
    g_started++;
    for (volatile uint32_t i = 0; i < 2000; i++) { __asm volatile("nop"); }
}

static void vChecker(void *pv);

/* Schedule table */
static TimelineTaskConfig_t my_schedule[] = {
    {"CHECK",   vChecker,  SOFT_RT,   1,    500, 0, 256, 0, NULL, TASK_NOT_STARTED},

    {"T1",     vTinyWork,  HARD_RT,  10,     40, 0, 256, 1, NULL, TASK_NOT_STARTED},
    {"T2",     vTinyWork,  HARD_RT,  60,     90, 0, 256, 2, NULL, TASK_NOT_STARTED},

    {"T3",     vTinyWork,  HARD_RT, 110,  140, 0, 256, 3, NULL, TASK_NOT_STARTED},
    {"T4",     vTinyWork,  HARD_RT, 160,  190, 0, 256, 4, NULL, TASK_NOT_STARTED},

    {"T5",     vTinyWork,  HARD_RT, 210,  240, 0, 256, 5, NULL, TASK_NOT_STARTED},
    {"T6",     vTinyWork,  HARD_RT, 260,  290, 0, 256, 6, NULL, TASK_NOT_STARTED},

    {"T7",     vTinyWork,  HARD_RT, 410,  440, 0, 256, 7, NULL, TASK_NOT_STARTED},
};

static void vChecker(void *pv)
{
    (void)pv;

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(6000);

    while (xTaskGetTickCount() < deadline)
    {
        //waits until other tasks are done
        if (g_started >= 8) break;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ASSERT_RTOS(g_started >= 8);

    qemu_exit(TEST_PASS);
    for (;;) {}
}

test_result_t run_test(void)
{
    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    static TimelineTaskConfig_t sched_copy[sizeof(my_schedule) / sizeof(my_schedule[0])];
    memcpy(sched_copy, my_schedule, sizeof(my_schedule));

    ASSERT(xPreprocessSchedule(sched_copy, numTasks, SF_MS) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched_copy, numTasks, SF_MS, TOTAL_SF) == SCHED_VALID);

    vTestPlatformBringUp(true, my_schedule, numTasks);
    vStartTimelineScheduler(my_schedule, numTasks, SF_MS, TOTAL_SF);

    //never reached
    return TEST_FAIL;
}
