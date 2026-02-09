#include "test_common.h"

#include "FreeRTOS.h"
#include "task.h"

static volatile test_result_t g_result = TEST_RUNNING;

void vTestInit(void)
{
    g_result = TEST_RUNNING;
}

test_result_t xTestGetResult(void)
{
    return g_result;
}

void vTestPass(void)
{
    g_result = TEST_PASS;
    taskDISABLE_INTERRUPTS();
}

void vTestFail(void)
{
    g_result = TEST_FAIL;
    taskDISABLE_INTERRUPTS();
}
