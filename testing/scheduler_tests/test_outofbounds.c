#include <stdint.h>
#include "testing/commons/test_common.h"
#include "timeline_scheduler.h"

#define ASSERT_EQ(a,b) TEST_ASSERT((a) == (b))

//TEMPORARY AI GENERATED TEST

test_result_t run_test(void)
{
    TimelineTaskConfig_t sched[] = {
        // Crosses boundary: subframe=10ms, start=8ms end=12ms => invalid
        { "X", (TaskFunction_t)0, HARD_RT, 8, 12, 0, 128, 0, TASK_NOT_STARTED },
    };

    const uint32_t subframe_ms = 10;
    SchedError_t e = xPreprocessSchedule(sched, 1, subframe_ms);
    ASSERT_EQ(e, ERR_OUT_OF_BOUNDS);

    return TEST_PASS;
}
