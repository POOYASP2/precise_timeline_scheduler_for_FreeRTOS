#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"

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
    vTestPlatformBringUp(true);

    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));
    const uint32_t subFrameCount = TOTAL_SF;

    ASSERT(xPreprocessSchedule(my_schedule, numTasks, SF_MS) == SCHED_VALID);
    ulSubFrameDuration = SF_MS;

    extern SchedError_t xValidateSchedule(const TimelineTaskConfig_t*, uint32_t, uint32_t, uint32_t);
    ASSERT(xValidateSchedule(my_schedule, numTasks, SF_MS, subFrameCount) == SCHED_VALID);

    vStartTimelineScheduler(my_schedule, numTasks, SF_MS, subFrameCount);

    //never reached
    return TEST_FAIL;
}
