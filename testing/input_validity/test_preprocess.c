#include <stdint.h>
#include "testing/commons/test_common.h"
#include "timeline_scheduler.h"

static TimelineTaskConfig_t make_task(const char* name,
                                      TaskType_t type,
                                      uint32_t abs_start_ms,
                                      uint32_t abs_end_ms)
{
    TimelineTaskConfig_t t;
    t.task_name = name;
    t.function = (TaskFunction_t)0; //not used in these tests
    t.type = type;
    t.ulStart_time_ms = abs_start_ms;
    t.ulEnd_time_ms = abs_end_ms;
    t.ulSubframe_id = 0;
    t.usStackSize = 256;
    t.taskId = 0;
    t.xHandle = NULL;
    t.state = TASK_NOT_STARTED;
    return t;
}

static test_result_t subframe_boundary(void)
{

    const uint32_t SF = 100;

    TimelineTaskConfig_t sched[2];
    sched[0] = make_task("T1", HARD_RT, 12, 45);
    sched[1] = make_task("T2", HARD_RT, 120, 150);

    ASSERT(xPreprocessSchedule(sched, 2, SF), SCHED_VALID);

    TimelineTaskConfig_t badsched[2];
    badsched[0] = make_task("T1", HARD_RT, 12, 45);
    badsched[1] = make_task("T2", HARD_RT, 50, 150);

    ASSERT(xPreprocessSchedule(badsched, 2, SF), ERR_OUT_OF_BOUNDS);

    return TEST_PASS;
}

static test_result_t subframe_assignment(void)
{

    const uint32_t SF = 100;

    TimelineTaskConfig_t sched[2];
    sched[0] = make_task("T1", HARD_RT, 120, 145);
    sched[1] = make_task("T2", HARD_RT, 220, 245);

    ASSERT(xPreprocessSchedule(sched, 2, SF), SCHED_VALID);

    ASSERT(sched[0].ulSubframe_id, 1u);
    ASSERT(sched[0].ulStart_time_ms, 20u);
    ASSERT(sched[0].ulEnd_time_ms, 45u);
    ASSERT(sched[1].ulSubframe_id, 2u);
    ASSERT(sched[1].ulStart_time_ms, 20u);
    ASSERT(sched[1].ulEnd_time_ms, 45u);

    return TEST_PASS;
}

static test_result_t end_time_is_subframe_end(void)
{

    const uint32_t SF = 100;

    TimelineTaskConfig_t sched[1];
    sched[0] = make_task("T1", HARD_RT, 110, 200);

    ASSERT(xPreprocessSchedule(sched, 1, SF), SCHED_VALID);

    ASSERT(sched[0].ulSubframe_id, 1u);
    ASSERT(sched[0].ulStart_time_ms, 10u);
    ASSERT(sched[0].ulEnd_time_ms, 100u);

    return TEST_PASS;
}

test_result_t run_test(void)
{

    ASSERT(subframe_boundary(), TEST_PASS);
    ASSERT(subframe_assignment(), TEST_PASS);
    ASSERT(end_time_is_subframe_end(), TEST_PASS);

    return TEST_PASS;
}