#include "timeline_scheduler.h"
#include <stdio.h> 

/* Global variables to hold the schedule state */
static const TimelineTaskConfig_t *pxCurrentSchedule = NULL;
static uint32_t ulScheduleSize = 0;
static uint32_t ulSubFrameSize = 0;

void vStartTimelineScheduler(const TimelineTaskConfig_t *pxScheduleTable, 
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

    /* Step 2: Validate the Schedule (Placeholder) */
    // We will write the overlap checks here later. We will implement a system which checks all the params of
    // the table and detects any possible errors. It's somehow sth like feasibility check   
    
    /* Step 3: We will loop through the table and call xTaskCreate here later. */
    // In this section, if there is no error in feasibility and scheduler table, we create tasks with the params
    // we received from main.c (we create it using xTaskCreate() in which all the HRTs have the same priority)

    /* Step 4: We have to create a linked list for each subframe */
    // each sub-frame includes some tasks (Let's consider only HRTs for now). We have to put them in order based on
    // their deadline to execute them in a time-line based priodic scheduler. So we have to calculate minor and major
    // cycles and then divide our subframe to these cycles. Then, create a linked list which sorts the tasks to be executed.

    /* Step 5: we have to design a function which decides about tasks*/
    // It loops through the scheduler table and switches to the most qualified task for execuring. We will call this function
    // inside xTaskIncrementTick() function to decide about tasks every tick (1 ms)

    /* Step 6: Debug Print */
    // Use standard printf if it works in QEMU
    // printf("Scheduler Initialized! Total Tasks: %d\n", ulTableSize);
    
    /* Step 4: Start the Standard FreeRTOS Scheduler */
    vTaskStartScheduler();
}