#include "testing/commons/scheduler_common.h"
#include <string.h>

#define SF_MS    20u
#define TOTAL_SF 3u

static TimelineTaskConfig_t g_sched[3];
static int counter = 0;

static void vStartTask(void *pv)
{
    (void)pv;
    for (volatile uint32_t i = 0; i < (500000)*2; i++)
    {
        __asm volatile("nop");
    }
    ASSERT_RTOS(g_sched[1].state == TASK_NOT_STARTED);
    ASSERT_RTOS(g_sched[2].state == TASK_NOT_STARTED);
}

static void vGenericTask(void *pv)
{
    (void)pv;
    for (volatile uint32_t i = 0; i < (500000)*2; i++)
    {
        __asm volatile("nop");
    }
    ASSERT_RTOS(g_sched[0].state == TASK_DONE);
}

static void vCounter(void *pv)
{
    (void)pv;
    ASSERT_RTOS(g_sched[1].state == TASK_DONE);

    if (counter++ >= 5) {
        qemu_exit(TEST_PASS);
        for (;;) {}
    }
}

test_result_t run_test(void)
{
    g_sched[0] = (TimelineTaskConfig_t){
        .task_name="Start", .function=vStartTask, .type=HARD_RT,
        .ulStart_time_ms=0, .ulEnd_time_ms=19, .usStackSize=256,
        .taskId=1, .state=TASK_NOT_STARTED
    };

    g_sched[1] = (TimelineTaskConfig_t){
        .task_name="Generic Task", .function=vGenericTask, .type=HARD_RT,
        .ulStart_time_ms=20, .ulEnd_time_ms=39, .usStackSize=256,
        .taskId=2, .state=TASK_NOT_STARTED
    };

    g_sched[2] = (TimelineTaskConfig_t){
        .task_name="Counter", .function=vCounter, .type=HARD_RT,
        .ulStart_time_ms=40, .ulEnd_time_ms=60, .usStackSize=256,
        .taskId=3, .state=TASK_NOT_STARTED
    };
    
    TimelineTaskConfig_t sched_copy[3];
    memcpy(sched_copy, g_sched, sizeof(g_sched));

    ASSERT(xPreprocessSchedule(sched_copy, 3, SF_MS) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched_copy, 3, SF_MS, TOTAL_SF) == SCHED_VALID);

    vTestPlatformBringUp(true, g_sched, 3);
    vStartTimelineScheduler(g_sched, 3, SF_MS, TOTAL_SF);


    //never reached
    return TEST_FAIL;
}
