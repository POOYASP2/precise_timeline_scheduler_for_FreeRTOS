#include "testing/commons/scheduler_common.h"
#include <string.h>

#define SF_MS    20u
#define TOTAL_SF 3u

static TimelineTaskConfig_t g_sched[3];

//to force a deadline miss
static void vLongBlockingTask()
{
    ASSERT_RTOS(g_sched[1].state == TASK_RUNNING);
    vTaskDelay(pdMS_TO_TICKS(50));
    ASSERT_RTOS(eTaskGetState(g_sched[2].xHandle) == eReady);
}

static void vWorkingTask()
{
    ASSERT_RTOS(g_sched[2].state == TASK_RUNNING);
}

static void vChecker(void *pv)
{
    (void)pv;

    ASSERT_RTOS(g_sched[0].state == TASK_RUNNING);
    ASSERT_RTOS(g_sched[1].state == TASK_DEADLINE_MISSED);
    ASSERT_RTOS(g_sched[2].state == TASK_DONE);
    ASSERT_RTOS(eTaskGetState(g_sched[1].xHandle) == eSuspended);
    ASSERT_RTOS(eTaskGetState(g_sched[2].xHandle) == eSuspended);

    qemu_exit(TEST_PASS);
    for (;;) {}
}

test_result_t run_test(void)
{
    g_sched[0] = (TimelineTaskConfig_t){
        .task_name="CHECK", .function=vChecker, .type=HARD_RT,
        .ulStart_time_ms=21, .ulEnd_time_ms=40, .usStackSize=configMINIMAL_STACK_SIZE+256,
        .taskId=1, .state=TASK_NOT_STARTED
    };

    g_sched[1] = (TimelineTaskConfig_t){
        .task_name="MISS", .function=vLongBlockingTask, .type=HARD_RT,
        .ulStart_time_ms=1, .ulEnd_time_ms=3, .usStackSize=configMINIMAL_STACK_SIZE+128,
        .taskId=2, .state=TASK_NOT_STARTED
    };

    g_sched[2] = (TimelineTaskConfig_t){
        .task_name="DONE", .function=vWorkingTask, .type=HARD_RT,
        .ulStart_time_ms=5, .ulEnd_time_ms=20, .usStackSize=configMINIMAL_STACK_SIZE+128,
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
