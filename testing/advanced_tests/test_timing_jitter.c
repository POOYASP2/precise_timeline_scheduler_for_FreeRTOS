#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"

/* * ============================================================================
 * TEST SCENARIO: TIMING JITTER & EMPTY SLOTS
 * ============================================================================
 * Objective: 
 * Verify that the scheduler maintains accurate global time tracking (tick count)
 * even when intermediate sub-frames are completely empty (no tasks assigned).
 *
 * Configuration:
 * - Major Frame: 60ms
 * - Sub-frame:   20ms
 * - Structure:
 * SF 0 (0-20ms):  Task A runs.
 * SF 1 (20-40ms): EMPTY (No tasks).
 * SF 2 (40-60ms): Task B runs.
 *
 * Pass Condition: 
 * Task B must start at tick 40 (with a tolerance of +/- 1 tick).
 *
 * Fail Condition: 
 * Task B starts significantly early (fast-forwarding empty slots) or late.
 * ============================================================================
 */

#define MAJOR_MS   60u
#define SF_MS      20u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

/* * Helper Function: Integer to String Conversion 
 */
void simple_itoa(int num, char* str) {
    int i = 0;
    if (num == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    while (num != 0) {
        int rem = num % 10;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/10;
    }
    str[i] = '\0';
    
    // Reverse string
    int start = 0, end = i - 1;
    while (start < end) {
        char temp = str[start]; str[start] = str[end]; str[end] = temp;
        start++; end--;
    }
}

// --- TASK FUNCTIONS ---

// Dummy task for Sub-frame 0
static void vTaskA(void *pv) 
{ 
    (void)pv; 
}

// Measurement task for Sub-frame 2 (Target Start: 40ms)
static void vTaskB(void *pv) 
{ 
    (void)pv;
    char num_buf[32];
    
    // Capture current tick count
    TickType_t current_tick = xTaskGetTickCount();
    
    UART_printf("\r\n[TEST] Task B started at tick: ");
    simple_itoa((int)current_tick, num_buf);
    UART_printf(num_buf);
    UART_printf("\r\n");
    
    UART_printf("[TEST] Expected start tick: 40\r\n");

    // Check: 40ms +/- 1 tick tolerance (Range: 39 - 41)
    if (current_tick >= 39 && current_tick <= 41) 
    {
        UART_printf("[TEST] SUCCESS: Timing is accurate.\r\n");
        qemu_exit(TEST_PASS);
    } 
    else 
    {
        UART_printf("[TEST] FAIL: Significant timing drift detected!\r\n");
        qemu_exit(TEST_FAIL);
    }
    
    for(;;);
}

// --- SCHEDULE DEFINITION ---
static TimelineTaskConfig_t test_schedule[] = {
    // SF 0 (0-20ms) Task
    {"Task_A", vTaskA, HARD_RT, 5, 15, 0, 256, 1, NULL, TASK_NOT_STARTED},
    
    // SF 1 (20-40ms) -> INTENTIONALLY EMPTY
    
    // SF 2 (40-60ms) Task (Start time 40ms automatically assigns to SF 2)
    {"Task_B", vTaskB, HARD_RT, 40, 50, 0, 256, 2, NULL, TASK_NOT_STARTED},
};

// --- TEST ENTRY POINT ---
test_result_t run_test(void)
{
    const uint32_t numTasks = 2;

    vTestPlatformBringUp(true, test_schedule, numTasks);
    vStartTimelineScheduler(test_schedule, numTasks, SF_MS, TOTAL_SF);

    return TEST_FAIL; // Should not be reached
}
