#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"
#include <string.h>

#define MAJOR_MS   5000u
#define SF_MS      1000u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

int hrt_executed = 0;

static void vLongSRT(void *pv)
{
    (void)pv;

    for (volatile uint32_t i = 0; i < (500000000); i++)
    {
        __asm volatile("nop");
    }

    if (hrt_executed == 0) {
        qemu_exit(TEST_FAIL);
        for (;;) {}
    } else {
        qemu_exit(TEST_PASS);
        for (;;) {}
    }
}

static void vHRT(void *pv)
{
    (void)pv;

    hrt_executed++;
}

//hard rt starts and ends within soft rt timeline.
static TimelineTaskConfig_t my_schedule[] = {
    {"SRT_LONG",    vLongSRT, SOFT_RT, 10,  900, 0, 256, 0, NULL, TASK_NOT_STARTED},
    {"HRT_PREEMPT", vHRT,     HARD_RT, 20, 300, 0, 256, 1, NULL, TASK_NOT_STARTED},
};

test_result_t run_test(void)
{

    TimelineTaskConfig_t sched_copy[2];
    memcpy(sched_copy, my_schedule, sizeof(my_schedule));

    ASSERT(xPreprocessSchedule(sched_copy, 2, SF_MS) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched_copy, 2, SF_MS, TOTAL_SF) == SCHED_VALID);

    vTestPlatformBringUp(true, my_schedule, 3);

    vStartTimelineScheduler(my_schedule, 2, SF_MS, TOTAL_SF);


    //never reached
    return TEST_FAIL;
}
