#include "testing/commons/scheduler_common.h"

#define SF_MS    20u
#define TOTAL_SF 3u

static TimelineTaskConfig_t g_sched[2];

static void vDummy(void *pv) { (void)pv; vTaskDelay(pdMS_TO_TICKS(1)); }

static void vOrchestrator(void *pv)
{
    (void)pv;

    ulCurrentSubFrameIndex = 1;
    ulGlobalTimeInFrame    = 12;

    (void)xUpdateTimelineScheduler();
    vTaskDelay(pdMS_TO_TICKS(1));

    //(uses ulGlobalTimeInFrame instead of time-in-subframe, might be mistake)
    ASSERT_RTOS(g_sched[1].state == TASK_RUNNING);

    qemu_exit(TEST_PASS);
    for (;;) {}
}

test_result_t run_test(void)
{
    g_sched[0] = (TimelineTaskConfig_t){
        .task_name="ORCH", .function=vOrchestrator, .type=HARD_RT,
        .ulStart_time_ms=0, .ulEnd_time_ms=9, .usStackSize=configMINIMAL_STACK_SIZE+256,
        .taskId=1, .state=TASK_NOT_STARTED
    };

    g_sched[1] = (TimelineTaskConfig_t){
        .task_name="SF1", .function=vDummy, .type=HARD_RT,
        .ulStart_time_ms=12, .ulEnd_time_ms=15, .usStackSize=configMINIMAL_STACK_SIZE+128,
        .taskId=2, .state=TASK_NOT_STARTED
    };

    vTestPlatformBringUp(true);

    ASSERT(xPreprocessSchedule(g_sched, 2, SF_MS) == SCHED_VALID);
    ulSubFrameDuration = SF_MS;

    extern SchedError_t xValidateSchedule(const TimelineTaskConfig_t*, uint32_t, uint32_t, uint32_t);
    ASSERT(xValidateSchedule(g_sched, 2, SF_MS, TOTAL_SF) == SCHED_VALID);

    vStartTimelineScheduler(g_sched, 2, SF_MS, TOTAL_SF);

    //never reached
    return TEST_FAIL;
}
