#include "timeline_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include "trace.h"

// A structure for single subframe to store the tasks
typedef struct
{
    uint32_t ulTaskCount;              // How many tasks in this sub-frame
    TimelineTaskConfig_t **FrameTasks; // Array of pointers to the tasks
} SubFrameList_t;

SubFrameList_t *pxSubframeTable = NULL;


// Supervisor Variables
static TaskHandle_t xSupervisorHandle = NULL;
static volatile TimelineTaskConfig_t *pxTaskToReset = NULL; // pointer to structure

/* Global variables to hold the schedule state */
static TimelineTaskConfig_t *pxCurrentSchedule = NULL;
static uint32_t ulScheduleSize = 0;
static uint32_t ulSubFrameSize = 0;

// Handle for the first SRT task of the chain
static TaskHandle_t xFirstSRTHandle = NULL;

/*
 * INTERNAL VALIDATION FUNCTION
 * This function is static (private) to hide validation logic from the user (Encapsulation).
 * It checks for:
 * 1. Invalid Subframe IDs
 * 2. Invalid Timings (Start >= End)
 * 3. Boundary Violations (Task exceeds subframe duration)
 * 4. Task Overlaps (Only for Hard Real-Time tasks)
 */
SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule,
					uint32_t uxTaskCount,
					uint32_t ulSubFrameDuration,
					uint32_t ulTotalSubFrames)
{
	for (uint32_t i = 0; i < uxTaskCount; i++)
    	{
		if (pxSchedule[i].ulSubframe_id >= ulTotalSubFrames)
        	{
            		return ERR_INVALID_SF;
        	}

        	if (pxSchedule[i].type == SOFT_RT)
        	{
            		continue; // Skip to next task, SRTs don't need time checks
        	}

        	if (pxSchedule[i].ulEnd_time_ms <= pxSchedule[i].ulStart_time_ms)
        	{
            		return ERR_INVALID_TIME;
        	}

        	if (pxSchedule[i].ulEnd_time_ms > ulSubFrameDuration ||
            		pxSchedule[i].ulStart_time_ms > ulSubFrameDuration)
        	{
            		return ERR_OUT_OF_BOUNDS;
        	}

        	if (pxSchedule[i].type == HARD_RT)
        	{
            		for (uint32_t j = i + 1; j < uxTaskCount; j++)
            		{
                		if (pxSchedule[j].type == HARD_RT &&
                    			pxSchedule[i].ulSubframe_id == pxSchedule[j].ulSubframe_id)
                		{
                    			if ((pxSchedule[i].ulStart_time_ms < pxSchedule[j].ulEnd_time_ms) &&
                        			(pxSchedule[j].ulStart_time_ms < pxSchedule[i].ulEnd_time_ms))
                    			{
                        			return ERR_OVERLAP;
                    			}
                		}
            		}
        	}
    	}

    return SCHED_VALID;
}

void vTaskWrapper(void *pvParameters)
{
    TimelineTaskConfig_t *pxTask = (TimelineTaskConfig_t *)pvParameters;

    /* TaskId mapping: assume schedule index stored elsewhere is not available.
       Quick approach: use (task_name[5] - '1') is fragile.
       Better: add a field in config for stable id.
       For now: use 0 for TASK1, 1 for TASK2 based on pointer match is not possible here.
       So we log with taskId=0xEE as "unknown" until you add an id field.
    */
    const uint8_t taskId = pxTask->taskId;

    if (pxTask->type == HARD_RT){

        while (1)
        {
            if (pxTask->function != NULL)
            {
                pxTask->function(NULL);
            }

            pxTask->state = TASK_DONE;

            /* log completion (frame/subframe from globals) */
            extern volatile uint32_t ulCurrentSubFrameIndex;
            extern volatile uint32_t ulGlobalTimeInFrame;

            TracePushTimeline(pxTask->taskId,
                            TRACE_COMPLETE,
                            (uint16_t)ulGlobalTimeInFrame,
                            (uint8_t)ulCurrentSubFrameIndex,
                            0);

            vTaskSuspend(NULL);
        }

    }
    else
    {

        while (1)
        {
            // Wait for Start Signal (Notify is given from scheduler OR previous task)
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	        extern volatile uint32_t ulCurrentSubFrameIndex;
	        extern volatile uint32_t ulGlobalTimeInFrame;

	        TracePushTimeline(pxTask->taskId, TRACE_START, (uint16_t)ulGlobalTimeInFrame,
						     (uint8_t)ulCurrentSubFrameIndex, 0);

            if (pxTask->function != NULL){
                pxTask->function(NULL);
            } 

	        TracePushTimeline(pxTask->taskId,
                             TRACE_COMPLETE,
                             (uint16_t)ulGlobalTimeInFrame,
                             (uint8_t)ulCurrentSubFrameIndex,
                             0);

            // Wake the Next Task (if it exists)
            if (pxTask->pxNextSRT != NULL) 
            {
                TimelineTaskConfig_t *pNext = (TimelineTaskConfig_t *)pxTask->pxNextSRT;
                xTaskNotifyGive(pNext->xHandle);
            }
            
        }
        

    }

    
}

void vSupervisorTask(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {

        // Wait until notified by scheduler
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (pxTaskToReset != NULL)
        {

            vTaskDelete(pxTaskToReset->xHandle); // Kill the task

            // Recreate the task
            BaseType_t uxPriority;

            if (pxTaskToReset->type == HARD_RT)
            {
                uxPriority = HRT_PRIORITY;
            }
            else
            {
                uxPriority = SRT_PRIORITY;
            }

            xTaskCreate(
                vTaskWrapper,
                pxTaskToReset->task_name,
                pxTaskToReset->usStackSize,
                (void *)pxTaskToReset,
                uxPriority,
                (TaskHandle_t *const)&pxTaskToReset->xHandle);

            if (pxTaskToReset->xHandle != NULL)
            {
                // Immediately suspend the task
                vTaskSuspend(pxTaskToReset->xHandle);
            }

            // Reset pointer
            pxTaskToReset = NULL;
        }
    }
}

/* Calculate subframes, convert absolute time to relative, complain if crossing boundaries */
SchedError_t xPreprocessSchedule(TimelineTaskConfig_t *pxSchedule, 
                                 uint32_t uxTaskCount, 
                                 uint32_t ulSubFrameDuration)
{
    for(uint32_t i = 0; i < uxTaskCount; i++)
    {

        // Ignore SRT tasks completely in preprocessing
        if (pxSchedule[i].type == SOFT_RT) {
            pxSchedule[i].ulSubframe_id = 0; 
            pxSchedule[i].ulStart_time_ms = 0;
            pxSchedule[i].ulEnd_time_ms = 0;
            continue; 
        }


        uint32_t ulStart = pxSchedule[i].ulStart_time_ms;
        uint32_t ulEnd = pxSchedule[i].ulEnd_time_ms;

        // Calculate which frame the start and end belong to
        uint32_t ulStartFrame = ulStart / ulSubFrameDuration;
        // (ulEnd - 1) handles the exact boundary case.
        uint32_t ulEndFrame = (ulEnd == 0) ? 0 : (ulEnd - 1) / ulSubFrameDuration;

        // CHECK: Do they belong to different frames?
        if (ulStartFrame != ulEndFrame) {
            // Error: Task straddles a boundary (e.g., 8ms to 12ms)
            return ERR_OUT_OF_BOUNDS; 
        }

        pxSchedule[i].ulSubframe_id = ulStartFrame;

        // Convert Absolute Start to Relative Start
        pxSchedule[i].ulStart_time_ms = ulStart % ulSubFrameDuration;

        // Convert Absolute End to Relative End
        uint32_t mod_end = ulEnd % ulSubFrameDuration;
        if (mod_end == 0 && ulEnd != 0) {
            pxSchedule[i].ulEnd_time_ms = ulSubFrameDuration;
        } else {
            pxSchedule[i].ulEnd_time_ms = mod_end;
        }
    }

    return SCHED_VALID;
}

/* The main Scheduler Function*/
void vStartTimelineScheduler(TimelineTaskConfig_t *pxScheduleTable,
                             uint32_t ulTableSize,
                             uint32_t ulSubFrameDurationMs,
                             uint32_t ulTotalSubFrames)
{
    /*
     * PREPROCESS & VALIDATION (ENCAPSULATION)
     * We perform validation INSIDE the scheduler so main.c doesn't need to worry about it.
     * If an error occurs, we call a user-defined Hook Function (FreeRTOS style).
     */
    // Convert Absolute Times to Relative Times
    if(xPreprocessSchedule(pxScheduleTable, ulTableSize, ulSubFrameDurationMs) != SCHED_VALID)
    {
        // if the condition occurs, there's an error and we call the hook function.
	vApplicationScheduleErrorHook(ERR_PREPROCESS_FAIL);
        configASSERT(0); // then lock the system if hook returns
    }
    // Validate Schedule Logic (Overlaps, Bounds, etc.)
    SchedError_t xErr = xValidateSchedule(pxScheduleTable, ulTableSize, ulSubFrameDurationMs, ulTotalSubFrames);
    if(xErr != SCHED_VALID)
    {
	// an error occurs and we call again the hook
	vApplicationScheduleErrorHook(xErr);
	configASSERT(0);
    }

    // Step 1: Save the parameters (We will use later)
    pxCurrentSchedule = pxScheduleTable;
    ulScheduleSize = ulTableSize;
    ulSubFrameSize = ulSubFrameDurationMs;

    // A function to configure the time-related parameters of our scheduler
    vConfigureTimerForTimeline(ulSubFrameDurationMs, ulTotalSubFrames);

    /*Step2.2: Create subframe table for better implementation*/
    pxSubframeTable = (SubFrameList_t *)pvPortMalloc(sizeof(SubFrameList_t) * ulTotalSubFrames);

    // Initialize subframe table
    for (int i = 0; i < ulTotalSubFrames; i++)
    {
        pxSubframeTable[i].ulTaskCount = 0;
        pxSubframeTable[i].FrameTasks = NULL;
    }

    // Calculate the number of tasks for each subframe
    for (int i = 0; i < ulTableSize; i++)
    {
        if (pxScheduleTable[i].type == HARD_RT){
            uint32_t sub_index = pxScheduleTable[i].ulSubframe_id;
            pxSubframeTable[sub_index].ulTaskCount++;

        }
    }

    // Fill the table
    for (int i = 0; i < ulTotalSubFrames; i++)
    {
        if (pxSubframeTable[i].ulTaskCount > 0)
        {
            pxSubframeTable[i].FrameTasks = (TimelineTaskConfig_t **)pvPortMalloc(sizeof(TimelineTaskConfig_t *) * pxSubframeTable[i].ulTaskCount);

            uint32_t index = 0;

            for (int j = 0; j < ulTableSize; j++)
            {
                if (i == pxScheduleTable[j].ulSubframe_id && pxScheduleTable[j].type == HARD_RT)
                { // If the IDs match, copy the structure to FrameTask

                    pxSubframeTable[i].FrameTasks[index] = &pxScheduleTable[j];
                    index++;
                }
            }
        }
    }

    /* Step 3: We will loop through the table and call xTaskCreate here . */
    // In this section, if there is no error in feasibility and scheduler table, we create tasks with the params
    // we received from main.c (we create it using xTaskCreate() in which all the HRTs have the same priority)

    TimelineTaskConfig_t *pFirstSRT = NULL;
    TimelineTaskConfig_t *pPrevSRT = NULL;

    for (int i = 0; i < ulTableSize; i++)
    {

        UBaseType_t uxPriority;
        if (pxScheduleTable[i].type == HARD_RT)
        {
            uxPriority = HRT_PRIORITY;
        }
        else
        {
            uxPriority = SRT_PRIORITY;
        }

        xTaskCreate(
            vTaskWrapper,
            pxScheduleTable[i].task_name,
            pxScheduleTable[i].usStackSize,
            &pxScheduleTable[i], // Pass the structure itself
            uxPriority,
            &pxScheduleTable[i].xHandle);

        // If the task is HRT, immediately suspend it until its start time reaches
        if (pxScheduleTable[i].type == HARD_RT){

            if (pxScheduleTable[i].xHandle != NULL)
            {
                vTaskSuspend(pxScheduleTable[i].xHandle); // Immediately suspend the task
            }
        }
        else{
            // Create SRT chain. This is a linked list of SRTs. Each SRT points to the next one

            // If this is the first SRT we found, save it
            if (pFirstSRT == NULL) {
                pFirstSRT = &pxScheduleTable[i];
                xFirstSRTHandle = pxScheduleTable[i].xHandle ;
            }

            // Link previous SRT to current one
            if (pPrevSRT != NULL) {
                pPrevSRT->pxNextSRT = &pxScheduleTable[i];
            }

            // Update Previous to be current task
            pPrevSRT = &pxScheduleTable[i];
            
            // Initialize Next to NULL (safety)
            pxScheduleTable[i].pxNextSRT = NULL;
        }
    }

    // Now give Notify to the fisrt SRT to be ready
    if (xFirstSRTHandle != NULL){
        xTaskNotifyGive(xFirstSRTHandle);
    }

    /* Step 5: we have to design a function which decides about tasks*/
    // It loops through the scheduler table and switches to the most qualified task for executing. We will call this function
    // inside xTaskIncrementTick() function to decide about tasks every tick (1 ms)
    // This task is xUpdateTimelineScheduler() function

    /* Step 6: Debug Print */
    // Use standard printf if it works in QEMU
    // printf("Scheduler Initialized! Total Tasks: %d\n", ulTableSize);

    // Create Supervisor Task
    xTaskCreate(
        vSupervisorTask,
        "Supervisor for missed tasks",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 1,
        &xSupervisorHandle);

    // We should check if there is any task at time = 0
    for (int i = 0; i < pxSubframeTable[0].ulTaskCount; i++)
    {
        if (pxSubframeTable[0].FrameTasks[i]->ulStart_time_ms == 0)
        {
            // We have to run this task
            pxSubframeTable[0].FrameTasks[i]->state = TASK_RUNNING;

            vTaskResume(pxSubframeTable[0].FrameTasks[i]->xHandle);
        }
    }

    /* Step 7: Start the Standard FreeRTOS Scheduler */
    vTaskStartScheduler();
}

BaseType_t xUpdateTimelineScheduler(void)
{

    // Extern global variables from task.c
    extern volatile uint32_t ulCurrentSubFrameIndex;
    extern volatile uint32_t ulGlobalTimeInFrame;
    extern volatile uint32_t ulSubFrameDuration; 
    extern volatile uint32_t ulTotalSubFrames;

    // Define a flag to notify xTaskIncrementTick() for context switching
    BaseType_t xContextSwitchRequired = pdFALSE;

    // Convert global time (e.g. 12ms) to relative time (2ms)
    uint32_t ulTimeInSubFrame = ulGlobalTimeInFrame % ulSubFrameDuration;

    // In boundaries, the deadline check for the task in previous subframe was ignored. So I add this line for the checking of boundaries
    if (ulTimeInSubFrame == 0 && ulGlobalTimeInFrame > 0) {
        
        uint32_t ulPrevIndex = (ulCurrentSubFrameIndex == 0) ? (ulTotalSubFrames- 1) : (ulCurrentSubFrameIndex - 1);
        SubFrameList_t *pxPrevBucket = &pxSubframeTable[ulPrevIndex];

        for (uint32_t i = 0; i < pxPrevBucket->ulTaskCount; i++) {
            TimelineTaskConfig_t *pxPrevTask = pxPrevBucket->FrameTasks[i];
            
            // Check against Duration (e.g. 1000)
            if (pxPrevTask->ulEnd_time_ms == ulSubFrameDuration) {
                if (pxPrevTask->state == TASK_RUNNING) {
                    pxPrevTask->state = TASK_DEADLINE_MISSED;
                    
                    // Log using Absolute Time (ulGlobalTimeInFrame) for clarity
                    TracePushTimelineFromISR(pxPrevTask->taskId, TRACE_DEADLINE_MISS, 
                                             (uint16_t)ulGlobalTimeInFrame, (uint8_t)ulPrevIndex, 0, NULL);
                    
                    pxTaskToReset = pxPrevTask;
                    vTaskNotifyGiveFromISR(xSupervisorHandle, NULL);
                    xContextSwitchRequired = pdTRUE;
                }
            }
        }
    }




    // Extract tasks from current subframe
    SubFrameList_t *pxCurrentSubframeTasks = &pxSubframeTable[ulCurrentSubFrameIndex];


    for (uint32_t i = 0; i < pxCurrentSubframeTasks->ulTaskCount; i++)
    {

        // Obtain first task of current subframe
        TimelineTaskConfig_t *pxTask = pxCurrentSubframeTasks->FrameTasks[i];

        // Check for running
        if ((pxTask->state == TASK_NOT_STARTED) &&
            (pxTask->ulStart_time_ms == ulTimeInSubFrame))
        {
            pxTask->state = TASK_RUNNING;

            /* LOG: START */
            TracePushTimelineFromISR(
                pxTask->taskId,
                TRACE_START,
                (uint16_t)ulGlobalTimeInFrame,
                (uint8_t)ulCurrentSubFrameIndex,
                0,
                NULL);

            if (xTaskResumeFromISR(pxTask->xHandle) == pdTRUE)
            {
                xContextSwitchRequired = pdTRUE;
            }
        }

        // Check for deadline
        if (pxTask->ulEnd_time_ms == ulTimeInSubFrame && pxTask->type == HARD_RT)
        {
            if (pxTask->state == TASK_RUNNING)
            {
                pxTask->state = TASK_DEADLINE_MISSED;

                /* LOG: DEADLINE_MISS */
                TracePushTimelineFromISR(
                    pxTask->taskId, 
                    TRACE_DEADLINE_MISS,
                    (uint16_t)ulGlobalTimeInFrame,
                    (uint8_t)ulCurrentSubFrameIndex,
                    0,
                    NULL);

                pxTaskToReset = pxTask;
                vTaskNotifyGiveFromISR(xSupervisorHandle, NULL);
                xContextSwitchRequired = pdTRUE;
            }
        }
    }

    return xContextSwitchRequired;
}

void vResetTimelineMajorFrame(void)
{
    for (uint32_t i = 0; i < ulScheduleSize; i++)
    {

        TimelineTaskConfig_t *pxTask = (TimelineTaskConfig_t *)&pxCurrentSchedule[i];
        pxTask->state = TASK_NOT_STARTED;
    }

    // We have to give notify to the first SRT task in the new major frame
    if(xFirstSRTHandle != NULL){
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(xFirstSRTHandle, &xHigherPriorityTaskWoken);
    }
}
