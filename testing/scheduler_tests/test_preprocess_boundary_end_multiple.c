#include <stdint.h>
#include "testing/commons/test_common.h"
#include "timeline_scheduler.h"

#define ASSERT_EQ(a,b) TEST_ASSERT((a) == (b))

//TEMPORARY AI GENERATED TEST

test_result_t run_test(void)
{
    TimelineTaskConfig_t sched[] = {
        // Start exactly at 10ms => subframe 1, relative start=0
        // End exactly at 20ms => relative end should become 10 (full subframe)
        { "Y", (TaskFunction_t)0, HARD_RT, 10, 20, 0, 128, 0, TASK_NOT_STARTED },
    };

    const uint32_t subframe_ms = 10;
    SchedError_t e = xPreprocessSchedule(sched, 1, subframe_ms);
    ASSERT_EQ(e, SCHED_VALID);

    ASSERT_EQ(sched[0].ulSubframe_id, 1);
    ASSERT_EQ(sched[0].ulStart_time_ms, 0);
    ASSERT_EQ(sched[0].ulEnd_time_ms, 10);

    return TEST_PASS;
}
