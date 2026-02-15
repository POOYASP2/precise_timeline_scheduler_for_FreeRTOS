#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"

#define MAJOR_MS   5000u
#define SF_MS      1000u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

//DEPENDS ON PREEMPTION DONT RUN YET

static volatile uint32_t g_started = 0;

static void vTinyWork(void *pv)
{
    (void)pv;
    g_started++;
    for (volatile uint32_t i = 0; i < 2000; i++) { __asm volatile("nop"); }
}

static void vOrchestrator(void *pv)
{
    (void)pv;

    // Wait up to ~1 major frame worth of time for all tasks to have run at least once.
    // ORCH is inside SF0 with a big window so it can wait.
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(6000);

    while (xTaskGetTickCount() < deadline)
    {
        // Each task increments g_started once when it runs.
        // Expect 8 total tasks to run at least once (including ORCH itself).
        if (g_started >= 8) break;
    }

    ASSERT_RTOS(g_started >= 8);

    // Stronger: verify all non-ORCH tasks reached DONE (depends on wrapper behavior)
    // We skip ORCH (index 0) because it’s the current task.
    for (uint32_t i = 1; i < (uint32_t)(sizeof(my_schedule)/sizeof(my_schedule[0])); i++) {
        ASSERT_RTOS(my_schedule[i].state == TASK_DONE);
    }

    qemu_exit(TEST_PASS);
    for (;;) {}
}

static TimelineTaskConfig_t my_schedule[] = {
    {"ORCH",   vOrchestrator, SOFT_RT,  1,  900, 0, 256,  0, NULL, TASK_NOT_STARTED},

    {"T1",     vTinyWork, HARD_RT, 10,  40,  0, 256,  1, NULL, TASK_NOT_STARTED},
    {"T2",     vTinyWork, HARD_RT, 60,  90,  0, 256,  2, NULL, TASK_NOT_STARTED},

    {"T3",     vTinyWork, HARD_RT, 1010, 1040, 0, 256, 3, NULL, TASK_NOT_STARTED},
    {"T4",     vTinyWork, HARD_RT, 1060, 1090, 0, 256, 4, NULL, TASK_NOT_STARTED},

    {"T5",     vTinyWork, HARD_RT, 2010, 2040, 0, 256, 5, NULL, TASK_NOT_STARTED},
    {"T6",     vTinyWork, HARD_RT, 2060, 2090, 0, 256, 6, NULL, TASK_NOT_STARTED},

    {"T7",     vTinyWork, HARD_RT, 4010, 4040, 0, 256, 7, NULL, TASK_NOT_STARTED},
};

test_result_t run_test(void)
{
    //move after startime when preemption works
    return TEST_FAIL;
    vTestPlatformBringUp(true);

    const uint32_t numTasks = (uint32_t)(sizeof(my_schedule) / sizeof(my_schedule[0]));

    ASSERT(xPreprocessSchedule(my_schedule, numTasks, SF_MS) == SCHED_VALID);
    ulSubFrameDuration = SF_MS;

    extern SchedError_t xValidateSchedule(const TimelineTaskConfig_t*, uint32_t, uint32_t, uint32_t);
    ASSERT(xValidateSchedule(my_schedule, numTasks, SF_MS, TOTAL_SF) == SCHED_VALID);

    vStartTimelineScheduler(my_schedule, numTasks, SF_MS, TOTAL_SF);
}
