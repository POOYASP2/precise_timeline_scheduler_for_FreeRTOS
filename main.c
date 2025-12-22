#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* ---------------------------------------------------
 * Test Task
 * --------------------------------------------------- */
void vTestTask(void *pvParameters) {
    (void)pvParameters; // Unused
    volatile uint32_t ulCycleCount = 0;

    for (;;) {
        // Just increment a counter so we can see it changing in GDB/QEMU
        ulCycleCount++;
        
        // Yield to let the idle task run (optional in preemptive, good practice)
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

/* ---------------------------------------------------
 * Main Entry Point
 * --------------------------------------------------- */
int main(void) {
    // 1. Create a simple task
    // Parameters: Function, Name, Stack Size, Params, Priority, Handle
    xTaskCreate(vTestTask, "Test", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // 2. Start the Scheduler (Should never return)
    vTaskStartScheduler();

    // 3. Trap if scheduler fails (e.g. not enough heap)
    for (;;) { }
}

/* ---------------------------------------------------
 * Required Hooks (Enabled in FreeRTOSConfig.h)
 * --------------------------------------------------- */

// Called if configCHECK_FOR_STACK_OVERFLOW is set
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    // Trap/Loop forever so we can catch it with debugger
    for (;;) { __asm("bkpt 0"); } 
}

// Called if configUSE_MALLOC_FAILED_HOOK is set
void vApplicationMallocFailedHook(void) {
    // Trap
    for (;;) { __asm("bkpt 0"); }
}

// Called if configUSE_TICK_HOOK is set
// This runs inside the interrupt! Keep it short.
void vApplicationTickHook(void) {
    // Do nothing for now
}

// Called by configASSERT
void vAssertCalled(const char *pcFileName, uint32_t ulLine) {
    (void)pcFileName;
    (void)ulLine;
    // Trap
    for (;;) { __asm("bkpt 0"); }
}

/* ========================================================== */
/* REQUIRED FOR configSUPPORT_STATIC_ALLOCATION = 1           */
/* ========================================================== */

/* 1. Static Memory for the Idle Task
   The kernel needs a Task Control Block (TCB) and a Stack. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* 2. Static Memory for the Timer Task
   Only required if configUSE_TIMERS is 1. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
