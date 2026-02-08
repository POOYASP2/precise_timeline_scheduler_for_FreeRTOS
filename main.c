#include "timeline_scheduler.h" 
#define MAJOR_FRAME_DURATION_MS 100
#define MINOR_FRAME_DURATION_MS 10

// A helper function to check any possible mistakes on the schedule table before scheduling
SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule, 
                               uint32_t uxTaskCount, 
                               uint32_t ulSubFrameDuration,
                               uint32_t ulTotalSubFrames);

/* Dummy Task Functions */
void vTask1(void *pvParams) { 
    
}
void vTask2(void *pvParams) { 
    
}

/* The Schedule Table */
const TimelineTaskConfig_t my_schedule[] = {
    // Name      Func         Type     Start  End   Slot  Stack
    { "TASK 1", vTask1, HARD_RT, 0,     5,    0,    128 },
    { "TASK 2", vTask2, HARD_RT, 5,     10,   0,    128 }
};

int main(void)
{
    // We should pass the major and minor frame durations during the execution to argv!
    // If the required checks are satisfied we start the scheduler
    uint32_t subFrameCount = MAJOR_FRAME_DURATION_MS / MINOR_FRAME_DURATION_MS;
    uint32_t numTasks = sizeof(my_schedule)/sizeof(my_schedule[0]);

    if(xValidateSchedule(my_schedule, numTasks, MINOR_FRAME_DURATION_MS, subFrameCount) == SCHED_VALID) //if the table valid proceed...
    {
	/* Initialize your Timeline Scheduler */
        // Pass the array, the number of tasks (2), subframe size (10ms), total slots (1)
    	vStartTimelineScheduler(my_schedule, numTasks, MINOR_FRAME_DURATION_MS, subFrameCount);
    }
    
    while(1){
        // infinite loop (never reach here)
    };
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
    // TrapTimelineTaskConfig_t
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
SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule, 
			     uint32_t uxTaskCount,
			     uint32_t ulSubFrameDuration,
			     uint32_t ulTotalSubFrames)
{
    for(uint32_t i=0; i < uxTaskCount; i++)
    {
	// verifying that the start time of the task is less than the end time!
	if (pxSchedule[i].ulEnd_time_ms <= pxSchedule[i].ulStart_time_ms) {
            // LOG!
            return ERR_INVALID_TIME;
        }
	// here returning an error if the end time and start time are exceeding the subframe duration
	if(pxSchedule[i].ulEnd_time_ms > ulSubFrameDuration || pxSchedule[i].ulStart_time_ms > ulSubFrameDuration)
	{
	    // LOG!
	    return ERR_OUT_OF_BOUNDS;
	}
	// a task cannot be asigned to a non existing subframe, here checking that!
	if (pxSchedule[i].ulSubframe_id >= ulTotalSubFrames) {
            // LOG!
            return ERR_INVALID_SF;
        }
	// check if the type of the task is HRT
	if(pxSchedule[i].type == HARD_RT)
	{   // if yes we'll check if a congestion occurs or not!
	    for (uint32_t j = i + 1; j < uxTaskCount; j++)
	   {
		// We'll compare between HRT tasks under the same subframe
		if (pxSchedule[j].type == HARD_RT && pxSchedule[i].ulSubframe_id == pxSchedule[j].ulSubframe_id) {
			// given tasks A and B
			// we'll have a congestion if start_A < end_B and start_B < end_A, we must check it:
			if ((pxSchedule[i].ulStart_time_ms < pxSchedule[j].ulEnd_time_ms) && 
                        (pxSchedule[j].ulStart_time_ms < pxSchedule[i].ulEnd_time_ms)) {
                        	// LOG!
                        	return ERR_OVERLAP;
			}
		}
	   }
	}

    }

	return SCHED_VALID;

}
