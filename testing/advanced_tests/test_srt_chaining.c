#include "testing/commons/scheduler_common.h"
#include "timeline_scheduler.h"
#include <string.h>

/* * ============================================================================
 * TEST SCENARIO: SRT CHAINING EXECUTION
 * ============================================================================
 * Objective: 
 * Verify that multiple Soft Real-Time (SRT) tasks execute sequentially 
 * within the idle time of a single sub-frame.
 *
 * Configuration:
 * - Major Frame: 100ms
 * - Sub-frame:   20ms
 * - Structure:
 * 1. HRT_Gate: Runs 0-5ms. Leaves 15ms of idle time in SF 0.
 * 2. SRT_1, SRT_2, SRT_3: Must run sequentially in the remaining idle time.
 *
 * Pass Condition: 
 * Execution order must be strictly: HRT -> SRT1 -> SRT2 -> SRT3.
 *
 * Fail Condition: 
 * Any SRT task is skipped, or the execution order is incorrect.
 * ============================================================================
 */

#define MAJOR_MS   100u
#define SF_MS      20u
#define TOTAL_SF   (MAJOR_MS / SF_MS)

// Global array to log the execution order of tasks
static int execution_log[10]; 
static int log_idx = 0;

/* * Helper Function: Integer to String Conversion 
 * Used to avoid dependency on snprintf/printf standard library overhead.
 */
void simple_itoa(int num, char* str) {
    int i = 0;
    int isNegative = 0;
  
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
  
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }
  
    while (num != 0) {
        int rem = num % 10;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/10;
    }
  
    if (isNegative)
        str[i++] = '-';
  
    str[i] = '\0';
  
    // Reverse string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// --- TASK FUNCTIONS ---

static void vHRT_Gate(void *pv) 
{ 
    (void)pv; 
    execution_log[log_idx++] = 1; // ID 1: HRT Executed
}

static void vSRT_1(void *pv) 
{ 
    (void)pv; 
    execution_log[log_idx++] = 2; // ID 2: SRT 1 Executed
}

static void vSRT_2(void *pv) 
{ 
    (void)pv; 
    execution_log[log_idx++] = 3; // ID 3: SRT 2 Executed
}

static void vSRT_3(void *pv) 
{ 
    (void)pv; 
    execution_log[log_idx++] = 4; // ID 4: SRT 3 Executed
}

// --- VERIFICATION TASK (Checker) ---
// This task is not part of the timeline schedule. It monitors the results.
static void vCheckerTask(void *pv)
{
    (void)pv;
    char num_buf[16]; // Buffer for string conversion
    
    // Wait until the end of the first sub-frame (plus buffer time)
    vTaskDelay(pdMS_TO_TICKS(30)); 

    UART_printf("\r\n[TEST] Execution Order Log: ");
    
    // Print the log to UART
    for(int i=0; i<log_idx; i++) {
        simple_itoa(execution_log[i], num_buf);
        UART_printf(num_buf);
        UART_printf(" -> ");
    }
    UART_printf("END\r\n");

    // Validate Sequence: 1 -> 2 -> 3 -> 4
    if (log_idx == 4 && 
        execution_log[0] == 1 && 
        execution_log[1] == 2 && 
        execution_log[2] == 3 && 
        execution_log[3] == 4) 
    {
        UART_printf("[TEST] SUCCESS: SRT Chain executed correctly.\r\n");
        qemu_exit(TEST_PASS);
    } 
    else 
    {
        UART_printf("[TEST] FAIL: SRT Chain broken or incomplete!\r\n");
        if (log_idx < 4) UART_printf("[TEST] Reason: Some tasks did not run.\r\n");
        qemu_exit(TEST_FAIL);
    }
    
    // Infinite loop if QEMU does not exit
    for(;;);
}

// --- SCHEDULE DEFINITION ---
static TimelineTaskConfig_t test_schedule[] = {
    // Name, Function, Type, Start, End, SubframeID, Stack, ID, Handle, State
    {"HRT_Gate", vHRT_Gate, HARD_RT, 0, 5, 0, 256, 1, NULL, TASK_NOT_STARTED},
    {"SRT_1",    vSRT_1,    SOFT_RT, 0, 0, 0, 256, 2, NULL, TASK_NOT_STARTED},
    {"SRT_2",    vSRT_2,    SOFT_RT, 0, 0, 0, 256, 3, NULL, TASK_NOT_STARTED},
    {"SRT_3",    vSRT_3,    SOFT_RT, 0, 0, 0, 256, 4, NULL, TASK_NOT_STARTED},
};

// --- TEST ENTRY POINT ---
test_result_t run_test(void)
{
    log_idx = 0;
    
    // Create the independent checker task
    xTaskCreate(vCheckerTask, "Checker", 256, NULL, configMAX_PRIORITIES-1, NULL);

    const uint32_t numTasks = 4;

    vTestPlatformBringUp(true, test_schedule, numTasks);
    vStartTimelineScheduler(test_schedule, numTasks, SF_MS, TOTAL_SF);

    return TEST_FAIL; // Should not be reached
}
