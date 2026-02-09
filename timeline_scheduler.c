#include "timeline_scheduler.h"
#include <stdio.h>
#include <stdlib.h>

/* Definitions */
#define HRT_PRIORITY (tskIDLE_PRIORITY + 2)
#define SRT_PRIORITY (tskIDLE_PRIORITY + 1)


// A structure for single subframe to store the tasks
typedef struct {
    uint32_t ulTaskCount;           // How many tasks in this sub-frame
    TimelineTaskConfig_t **FrameTasks; // Array of pointers to the tasks
} SubFrameList_t;

SubFrameList_t *pxSubframeTable = NULL;


//Supervisor Variables
static TaskHandle_t xSupervisorHandle = NULL;
static volatile TimelineTaskConfig_t *pxTaskToReset = NULL; // pointer to structure


/* Global variables to hold the schedule state */
static TimelineTaskConfig_t *pxCurrentSchedule = NULL;
static uint32_t ulScheduleSize = 0;
static uint32_t ulSubFrameSize = 0;


void vTaskWrapper(void *pvParameters){

    TimelineTaskConfig_t *pxTask = (TimelineTaskConfig_t *)pvParameters;
    
    while(1){
        // Run the related function to the task
        if (pxTask->function != NULL) {
            pxTask->function(NULL);
        }

        pxTask->state = TASK_DONE ;     // Update state to notify scheduler
        // get tick count here and fill the queue for logger (Here we can get ulGlobalTimeInFrame)
        // After finishing the job, we wait here until the scheduler Resumes us again
        vTaskSuspend(NULL);
    }

}


void vSupervisorTask(void *pvParameters){
    (void)pvParameters ;

    while(1){

        // Wait until notified by scheduler
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (pxTaskToReset != NULL){

            vTaskDelete(pxTaskToReset->xHandle);   // Kill the task
        
            // Recreate the task
            BaseType_t uxPriority;

            if (pxTaskToReset->type == HARD_RT){
                uxPriority = HRT_PRIORITY;
            }
            else{
                uxPriority = SRT_PRIORITY;
            }

            xTaskCreate(
                vTaskWrapper,
                pxTaskToReset->task_name,
                pxTaskToReset->usStackSize,
                (void *)pxTaskToReset,
                uxPriority,
                (TaskHandle_t *const)&pxTaskToReset->xHandle
            );

            if(pxTaskToReset->xHandle != NULL){
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
    for (uint32_t i = 0 ; i < ulTotalSubFrames ; i++) {
        pxSubframeTable[i].ulTaskCount = 0;
        pxSubframeTable[i].FrameTasks = NULL;
    }

    // Calculate the number of tasks for each subframe
    for (uint32_t i = 0 ; i < ulTableSize ; i++) {
        uint32_t sub_index = pxScheduleTable[i].ulSubframe_id ;
        pxSubframeTable[sub_index].ulTaskCount++;
    }

    // Fill the ppTasks
    for (uint32_t i = 0 ; i < ulTotalSubFrames ; i++) {
        if (pxSubframeTable[i].ulTaskCount > 0){
            pxSubframeTable[i].FrameTasks = (TimelineTaskConfig_t**)pvPortMalloc(sizeof(TimelineTaskConfig_t *) * pxSubframeTable[i].ulTaskCount);

            uint32_t index = 0;

            for (uint32_t j = 0 ; j < ulTableSize ; j++){
                if(i == pxScheduleTable[j].ulSubframe_id){ // If the IDs match, copy the structure to FrameTask

                    pxSubframeTable[i].FrameTasks[index] = &pxScheduleTable[j];
                    index++ ;
                }
            }
        }
    }


    /* Step 3: We will loop through the table and call xTaskCreate here later. */
    // In this section, if there is no error in feasibility and scheduler table, we create tasks with the params
    // we received from main.c (we create it using xTaskCreate() in which all the HRTs have the same priority)

    UBaseType_t uxPriority ;
    for (uint32_t i = 0 ; i < ulTableSize ; i++){

        if (pxScheduleTable[i].type == HARD_RT){
            uxPriority = HRT_PRIORITY;
        }
        else{
            uxPriority = SRT_PRIORITY;
        }

        xTaskCreate(
            vTaskWrapper,
            pxScheduleTable[i].task_name,
            pxScheduleTable[i].usStackSize,
            &pxScheduleTable[i],            // Pass the structure itself
            uxPriority,
            &pxScheduleTable[i].xHandle      
        );

        if (pxScheduleTable[i].xHandle != NULL){
            vTaskSuspend(pxScheduleTable[i].xHandle) ; // Immediately suspend the task
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
        &xSupervisorHandle
    );

    // We should check if there is any task at time = 0
    for (uint32_t i = 0 ; i < pxSubframeTable[0].ulTaskCount ; i++){
        if(pxSubframeTable[0].FrameTasks[i]->ulStart_time_ms == 0){
            // We have to run this task
            pxSubframeTable[0].FrameTasks[i]->state = TASK_RUNNING ;

            vTaskResume(pxSubframeTable[0].FrameTasks[i]->xHandle);
        }
    }



    /* Step 7: Start the Standard FreeRTOS Scheduler */
    vTaskStartScheduler();
}

BaseType_t xUpdateTimelineScheduler(void){

    // Extern global variables from task.c
    extern volatile uint32_t ulCurrentSubFrameIndex;
    extern volatile uint32_t ulGlobalTimeInFrame;
    extern volatile uint32_t ulSubFrameDuration; // Need this for modulo

    // Define a flag to notify xTaskIncrementTick() for context switching
    BaseType_t xContextSwitchRequired = pdFALSE;

    // Convert global time (e.g. 12ms) to relative time (2ms)
    uint32_t ulTimeInSubFrame = ulGlobalTimeInFrame % ulSubFrameDuration;

    // Extract tasks from current subframe
    SubFrameList_t *pxCurrentSubframeTasks = &pxSubframeTable[ulCurrentSubFrameIndex];

    /*
    if (ulGlobalTimeInFrame == 0 && ulCurrentSubFrameIndex == 0) {
        // ... (Loop through all tasks and reset state = TASK_NOT_STARTED) ...
        // We will do this later...
    }*/

    for (uint32_t i = 0 ; i < pxCurrentSubframeTasks->ulTaskCount ; i++){

        // Obtain first task of current subframe
        TimelineTaskConfig_t *pxTask = pxCurrentSubframeTasks->FrameTasks[i] ;

        // Check for running
        if(pxTask->ulStart_time_ms == ulTimeInSubFrame){

            pxTask->state = TASK_RUNNING ;
            if(xTaskResumeFromISR(pxTask->xHandle) == pdTRUE){
                xContextSwitchRequired = pdTRUE ;  // Force to context switch immediately
            }
        }

        // Check for deadline
        if(pxTask->ulEnd_time_ms == ulTimeInSubFrame){
            
            if (pxTask->state == TASK_DONE){
                // The task finished on time
                // log the message to the Queue (Take the saved time from wrapper and log)
            }
            else if(pxTask->state == TASK_RUNNING){
                pxTask->state = TASK_DEADLINE_MISSED;
                // Kill the task (Tell the supervisor to kill the task)

                pxTaskToReset = pxTask ;
                vTaskNotifyGiveFromISR(xSupervisorHandle, NULL);
                xContextSwitchRequired = pdTRUE ;
            }
        }

    }

    return xContextSwitchRequired;
}


void vResetTimelineMajorFrame(void) {

    for (uint32_t i = 0; i < ulScheduleSize; i++) {
        
        TimelineTaskConfig_t *pxTask = (TimelineTaskConfig_t *)&pxCurrentSchedule[i];
        
        pxTask->state = TASK_NOT_STARTED;

    }
}
