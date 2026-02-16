#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"
#include <string.h>

#define MAJOR_MS   5000u
#define SF_MS      1000u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

static void vBlockLong(void *pv)
{
    //forced miss
    (void)pv;
    vTaskDelay(pdMS_TO_TICKS(300));
}

static void vChecker(void *pv);

/* Schedule table (8 tasks)*/
static TimelineTaskConfig_t my_schedule[] = {
    {"CHECK", vChecker,  SOFT_RT,   1,   900,  0, 256, 0, NULL, TASK_NOT_STARTED},

    {"M1",    vBlockLong, HARD_RT,  10,   20,  0, 256, 1, NULL, TASK_NOT_STARTED},
    {"M2",    vBlockLong, HARD_RT,  30,   40,  0, 256, 2, NULL, TASK_NOT_STARTED},

    {"M3",    vBlockLong, HARD_RT, 1010, 1020, 0, 256, 3, NULL, TASK_NOT_STARTED},
    {"M4",    vBlockLong, HARD_RT, 1030, 1040, 0, 256, 4, NULL, TASK_NOT_STARTED},

    {"M5",    vBlockLong, HARD_RT, 2010, 2020, 0, 256, 5, NULL, TASK_NOT_STARTED},
    {"M6",    vBlockLong, HARD_RT, 2030, 2040, 0, 256, 6, NULL, TASK_NOT_STARTED},

    {"M7",    vBlockLong, HARD_RT, 3010, 3020, 0, 256, 7, NULL, TASK_NOT_STARTED},
};

static void vChecker(void *pv)
{
    (void)pv;

    const uint32_t n = (uint32_t)(sizeof(my_schedule)/sizeof(my_schedule[0]));
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(6000);

    while (xTaskGetTickCount() < deadline)
    {
        uint32_t missed = 0;
        for (uint32_t i = 1; i < n; i++) {
            if (my_schedule[i].state == TASK_DEADLINE_MISSED) missed++;
        }
        if (missed == (n - 1)) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    for (uint32_t i = 1; i < n; i++) {
        ASSERT_RTOS(my_schedule[i].state == TASK_DEADLINE_MISSED);
        ASSERT_RTOS(eTaskGetState(my_schedule[i].xHandle) == eSuspended);
    }

    qemu_exit(TEST_PASS);
    for (;;) {}
}

test_result_t run_test(void)
{
    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));


    static TimelineTaskConfig_t sched_copy[sizeof(my_schedule)/sizeof(my_schedule[0])];
    memcpy(sched_copy, my_schedule, sizeof(my_schedule));

    ASSERT(xPreprocessSchedule(sched_copy, numTasks, SF_MS) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched_copy, numTasks, SF_MS, TOTAL_SF) == SCHED_VALID);

    vTestPlatformBringUp(true, my_schedule, numTasks);
    vStartTimelineScheduler(my_schedule, numTasks, SF_MS, TOTAL_SF);

    //never reached
    return TEST_FAIL;
}
