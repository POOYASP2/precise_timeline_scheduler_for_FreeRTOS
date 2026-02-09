//to avoid multiple definition errors
#pragma once

typedef enum {
    TEST_RUNNING = 0,
    TEST_PASS,
    TEST_FAIL
} test_result_t;

void vTestInit(void);
void vTestPass(void);
void vTestFail(void);
test_result_t xTestGetResult(void);
