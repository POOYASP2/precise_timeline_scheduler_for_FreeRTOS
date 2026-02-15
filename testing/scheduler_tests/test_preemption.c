#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"

#define MAJOR_MS   5000u
#define SF_MS      1000u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

//DEPENDS ON PREEMPTION DONT RUN YET

static void vLongSRT(void *pv)
{
    (void)pv;

    for (volatile uint32_t i = 0; i < 2000000u; i++)
    {
        if ((i % 20000u) == 0u)
        {
            taskYIELD();
        }
    }

    //if hrt is never executed, preemption failed
    qemu_exit(TEST_FAIL);
    for (;;) {}
}

static void vHRT(void *pv)
{
    (void)pv;

    //if hrt runs, preemption was successful
    qemu_exit(TEST_PASS);
    for (;;) {}
}


static TimelineTaskConfig_t my_schedule[] = {
    {"SRT_LONG",    vLongSRT, SOFT_RT, 10,  900, 0, 256, 0, NULL, TASK_NOT_STARTED},
    {"HRT_PREEMPT", vHRT,     HARD_RT, 200, 300, 0, 256, 1, NULL, TASK_NOT_STARTED},
};

test_result_t run_test(void)
{
    vTestPlatformBringUp(true);

    ASSERT(xPreprocessSchedule(my_schedule, 2, SF_MS) == SCHED_VALID);

    extern SchedError_t xValidateSchedule(const TimelineTaskConfig_t*, uint32_t, uint32_t, uint32_t);
    ASSERT(xValidateSchedule(my_schedule, 2, SF_MS, TOTAL_SF) == SCHED_VALID);

    vStartTimelineScheduler(my_schedule, 2, SF_MS, TOTAL_SF);

    //never reached
    return TEST_FAIL;
}
