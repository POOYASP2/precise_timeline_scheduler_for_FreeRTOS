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

SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule,
                              uint32_t uxTaskCount,
                              uint32_t ulSubFrameDuration,
                              uint32_t ulTotalSubFrames);

static test_result_t invalid_time(void)
{

    const uint32_t SF = 100;
    const uint32_t TOTAL_SF = 5;

    TimelineTaskConfig_t sched[2];
    sched[0] = make_task("T1", HARD_RT, 0, 0);
    sched[1] = make_task("T2", HARD_RT, 10, 20);

    //preprocess first so validate expects relative times & subframe ids
    ASSERT(xPreprocessSchedule(sched, 2, SF) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched, 2, SF, TOTAL_SF) == ERR_INVALID_TIME);

    TimelineTaskConfig_t sched_2[1];
    sched[0] = make_task("T1", HARD_RT, 5, 0);

    ASSERT(xPreprocessSchedule(sched_2, 2, SF) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched_2, 2, SF, TOTAL_SF) == ERR_INVALID_TIME);

    return TEST_PASS;
}

static test_result_t subframe_boundary(void)
{

    const uint32_t SF = 100;
    const uint32_t TOTAL_SF = 5;

    TimelineTaskConfig_t sched[1];
    sched[0] = make_task("T1", HARD_RT, 10, 120);

    ASSERT(xValidateSchedule(sched, 1, SF, TOTAL_SF) == ERR_OUT_OF_BOUNDS);

    return TEST_PASS;
}

static test_result_t invalid_subframe_id(void)
{

    const uint32_t SF = 100;
    const uint32_t TOTAL_SF = 3;

    TimelineTaskConfig_t sched[1];
    sched[0] = make_task("T1", HARD_RT, 10, 20);

    ASSERT(xPreprocessSchedule(sched, 1, SF) == SCHED_VALID);

    sched[0].ulSubframe_id = 3;
    ASSERT(xValidateSchedule(sched, 1, SF, TOTAL_SF) == ERR_INVALID_SF);

    return TEST_PASS;
}

static test_result_t hrt_overlap(void)
{

    const uint32_t SF = 100;
    const uint32_t TOTAL_SF = 3;

    TimelineTaskConfig_t sched[2];
    sched[0] = make_task("T1", HARD_RT, 10, 30);
    sched[1] = make_task("T2", HARD_RT, 20, 40);

    ASSERT(xPreprocessSchedule(sched, 2, SF) == SCHED_VALID);
    ASSERT(xValidateSchedule(sched, 2, SF, TOTAL_SF) == ERR_OVERLAP);

    return TEST_PASS;
}

static test_result_t srt_hrt_overlap(void)
{

    const uint32_t SF = 100;
    const uint32_t TOTAL_SF = 3;

    {
        TimelineTaskConfig_t sched[2];
        sched[0] = make_task("HRT", HARD_RT, 10, 30);
        sched[1] = make_task("SRT", SOFT_RT, 20, 40);

        ASSERT(xPreprocessSchedule(sched, 2, SF) == SCHED_VALID);
        ASSERT(xValidateSchedule(sched, 2, SF, TOTAL_SF) == SCHED_VALID);
    }

    {
        TimelineTaskConfig_t sched_2[2];
        sched_2[0] = make_task("HRT1", HARD_RT, 10, 20);
        sched_2[1] = make_task("HRT2", HARD_RT, 20, 30);

        ASSERT(xPreprocessSchedule(sched_2, 2, SF) == SCHED_VALID);
        ASSERT(xValidateSchedule(sched_2, 2, SF, TOTAL_SF) == SCHED_VALID);
    }

    return TEST_PASS;
}


test_result_t run_test(void)
{

    ASSERT(invalid_time() == TEST_PASS);
    ASSERT(subframe_boundary() == TEST_PASS);
    ASSERT(invalid_subframe_id() == TEST_PASS);
    ASSERT(hrt_overlap() == TEST_PASS);
    ASSERT(srt_hrt_overlap() == TEST_PASS);

    return TEST_PASS;
}

