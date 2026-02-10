#include <stdint.h>
#include "testing/commons/test_common.h"
#include "timeline_scheduler.h"

#define ASSERT_EQ(a,b) TEST_ASSERT((a) == (b))

//TEMPORARY AI GENERATED TEST

test_result_t run_test(void)
{
    TimelineTaskConfig_t sched[] = {
        { "A", (TaskFunction_t)0, HARD_RT, 0,  5,  0, 128, 0, TASK_NOT_STARTED },
        { "B", (TaskFunction_t)0, HARD_RT, 5,  10, 0, 128, 0, TASK_NOT_STARTED },
        { "C", (TaskFunction_t)0, HARD_RT, 12, 15, 0, 128, 0, TASK_NOT_STARTED },
    };

    const uint32_t subframe_ms = 10;
    SchedError_t e = xPreprocessSchedule(sched, 3, subframe_ms);
    ASSERT_EQ(e, SCHED_VALID);

    // Task A: [0,5] in subframe 0 => relative unchanged
    ASSERT_EQ(sched[0].ulSubframe_id, 0);
    ASSERT_EQ(sched[0].ulStart_time_ms, 0);
    ASSERT_EQ(sched[0].ulEnd_time_ms, 5);

    // Task B: [5,10] end exactly at boundary => end becomes subframe duration (10)
    ASSERT_EQ(sched[1].ulSubframe_id, 0);
    ASSERT_EQ(sched[1].ulStart_time_ms, 5);
    ASSERT_EQ(sched[1].ulEnd_time_ms, 10);

    // Task C: [12,15] belongs to subframe 1 => start=2, end=5 relative
    ASSERT_EQ(sched[2].ulSubframe_id, 1);
    ASSERT_EQ(sched[2].ulStart_time_ms, 2);
    ASSERT_EQ(sched[2].ulEnd_time_ms, 5);

    return TEST_PASS;
}
