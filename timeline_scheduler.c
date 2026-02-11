#include "timeline_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include "trace.h"

/* Definitions */
#define HRT_PRIORITY (tskIDLE_PRIORITY + 2)
#define SRT_PRIORITY (tskIDLE_PRIORITY + 1)


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

void vTaskWrapper(void *pvParameters)
{
    TimelineTaskConfig_t *pxTask = (TimelineTaskConfig_t *)pvParameters;

    /* TaskId mapping: assume schedule index stored elsewhere is not available.
       Quick approach: use (task_name[5] - '1') is fragile.
       Better: add a field in config for stable id.
       For now: use 0 for TASK1, 1 for TASK2 based on pointer match is not possible here.
       So we log with taskId=0xEE as "unknown" until you add an id field.
    */
    const uint8_t taskId = 0xEE;

    for (;;)
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

/* The main Scheduler Function*/
void vStartTimelineScheduler(TimelineTaskConfig_t *pxScheduleTable,
                             uint32_t ulTableSize,
                             uint32_t ulSubFrameDurationMs,
                             uint32_t ulTotalSubFrames)
{
    // Step 1: Save the parameters (We will use later)
    pxCurrentSchedule = pxScheduleTable;
    ulScheduleSize = ulTableSize;
    ulSubFrameSize = ulSubFrameDurationMs;

    // A function to configure the time-related parameters of our scheduler
    vConfigureTimerForTimeline(ulSubFrameDurationMs, ulTotalSubFrames);

    /* Step 2.1: Validate the Schedule (Placeholder) */
    // We will write the overlap checks here later. We will implement a system which checks all the params of
    // the table and detects any possible errors. It's somehow sth like feasibility check

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
        uint32_t sub_index = pxScheduleTable[i].ulSubframe_id;
        pxSubframeTable[sub_index].ulTaskCount++;
    }

    // Fill the ppTasks
    for (int i = 0; i < ulTotalSubFrames; i++)
    {
        if (pxSubframeTable[i].ulTaskCount > 0)
        {
            pxSubframeTable[i].FrameTasks = (TimelineTaskConfig_t **)pvPortMalloc(sizeof(TimelineTaskConfig_t *) * pxSubframeTable[i].ulTaskCount);

            uint32_t index = 0;

            for (int j = 0; j < ulTableSize; j++)
            {
                if (i == pxScheduleTable[j].ulSubframe_id)
                { // If the IDs match, copy the structure to FrameTask

                    pxSubframeTable[i].FrameTasks[index] = &pxScheduleTable[j];
                    index++;
                }
            }
        }
    }

    /* Step 3: We will loop through the table and call xTaskCreate here later. */
    // In this section, if there is no error in feasibility and scheduler table, we create tasks with the params
    // we received from main.c (we create it using xTaskCreate() in which all the HRTs have the same priority)

    UBaseType_t uxPriority;
    for (int i = 0; i < ulTableSize; i++)
    {

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

        if (pxScheduleTable[i].xHandle != NULL)
        {
            vTaskSuspend(pxScheduleTable[i].xHandle); // Immediately suspend the task
        }
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

    // Define a flag to notify xTaskIncrementTick() for context switching
    BaseType_t xContextSwitchRequired = pdFALSE;

    // Extract tasks from current subframe
    SubFrameList_t *pxCurrentSubframeTasks = &pxSubframeTable[ulCurrentSubFrameIndex];

    /*
    if (ulGlobalTimeInFrame == 0 && ulCurrentSubFrameIndex == 0) {
        // ... (Loop through all tasks and reset state = TASK_NOT_STARTED) ...
        // We will do this later...
    }*/

    for (uint32_t i = 0; i < pxCurrentSubframeTasks->ulTaskCount; i++)
    {

        // Obtain first task of current subframe
        TimelineTaskConfig_t *pxTask = pxCurrentSubframeTasks->FrameTasks[i];

        // Check for running
        if ((pxTask->state == TASK_NOT_STARTED) &&
    (pxTask->ulStart_time_ms == ulGlobalTimeInFrame))
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
        if (pxTask->ulEnd_time_ms == ulGlobalTimeInFrame)
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
}
