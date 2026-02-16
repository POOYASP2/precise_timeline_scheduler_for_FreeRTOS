#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"
#include <string.h>

#define MAJOR_MS   500u
#define SF_MS      100u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

static void vCounterTask(void *pv)
{
    (void)pv;

    static uint32_t runs = 0;
    runs++;

    //the tasks will loop until counter reaches 5
    if (runs >= 5u)
    {
        qemu_exit(TEST_PASS);
        for (;;) {}
    }

    vTaskDelay(pdMS_TO_TICKS(1));
}

static void vTinyTask(void *pv)
{
    (void)pv;
    vTaskDelay(pdMS_TO_TICKS(1));
}


static TimelineTaskConfig_t my_schedule[] = {
    {"A",       vTinyTask,    SOFT_RT, 20, 25, 0, 256, 1, NULL, TASK_NOT_STARTED},
    {"B",       vTinyTask,    SOFT_RT, 120,125,0, 256, 2, NULL, TASK_NOT_STARTED},
    {"Counter", vCounterTask, HARD_RT, 300,340,  0, 256, 3, NULL, TASK_NOT_STARTED}
};

test_result_t run_test(void)
{

    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));
    const uint32_t subFrameCount = TOTAL_SF;

    ulSubFrameDuration = SF_MS;

    TimelineTaskConfig_t sched_copy[3];
    memcpy(sched_copy, my_schedule, sizeof(my_schedule));

    ASSERT(xPreprocessSchedule(sched_copy, 3, SF_MS) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched_copy, 3, SF_MS, TOTAL_SF) == SCHED_VALID);

    vTestPlatformBringUp(true, my_schedule, numTasks);
    vStartTimelineScheduler(my_schedule, numTasks, SF_MS, TOTAL_SF);

    //never reached
    return TEST_FAIL;
}
