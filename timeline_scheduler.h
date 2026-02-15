#ifndef TIMELINE_SCHEDULER_H
#define TIMELINE_SCHEDULER_H


#include "FreeRTOS.h"
#include "task.h"

// Define the Task States
typedef enum {
    TASK_NOT_STARTED = 0, // Default value
    TASK_RUNNING,
    TASK_DONE,
    TASK_DEADLINE_MISSED
} TimelineTaskStatus_t;

//Define the Task Types
typedef enum {
    HARD_RT,   // Must start/end exactly on time. Killed if late.Start other tasks if terminated earlier than deadline
    SOFT_RT    // Runs only if CPU is idle.
} TaskType_t;

/* Define the Error Codes */
typedef enum {
	SCHED_VALID = 0,
	ERR_INVALID_TIME, // Start >= End
	ERR_OUT_OF_BOUNDS, // End > Subframe Duration
	ERR_OVERLAP, // 2 HRT tasks overlap
	ERR_INVALID_SF, // non-existing sub-frame id!
	ERR_PREPROCESS_FAIL // absolute to relative conversion failed
} SchedError_t;

/* Define the Configuration Structure 
   This is what the user fills out in main.c to define their plan. */
typedef struct {
    const char* task_name;      // Human readable name (for debugging)
    TaskFunction_t function;    // Pointer to the function to run
    TaskType_t type;            // HRT or SRT

    uint32_t ulStart_time_ms;   // Relative to the start of the the task
    uint32_t ulEnd_time_ms;     // Relative to the end of the task
    
    uint32_t ulSubframe_id;     // Which subframe does this belong to?
    
    uint16_t usStackSize;       // Stack Size (Standard FreeRTOS param)

    uint8_t  taskId;

    TaskHandle_t xHandle;       // Scheduler will write this later
    volatile TimelineTaskStatus_t state ;
    void *pxNextSRT;            // For SRTs: Each task points to next SRT task
} TimelineTaskConfig_t;


/* Verifies the table and sets up the OS */
void vStartTimelineScheduler(TimelineTaskConfig_t *pxScheduleTable, 
                             uint32_t ulTableSize, 
                             uint32_t ulSubFrameDurationMs, 
                             uint32_t ulTotalSubFrames);

/* Converts absolute times to relative and checks for frame boundary violations */
SchedError_t xPreprocessSchedule(TimelineTaskConfig_t *pxSchedule, 
                                 uint32_t uxTaskCount, 
                                 uint32_t ulSubFrameDuration);


BaseType_t xUpdateTimelineScheduler(void);
void vResetTimelineMajorFrame(void) ;
void vApplicationScheduleErrorHook(SchedError_t xError);

SchedError_t xValidateSchedule(const TimelineTaskConfig_t *pxSchedule,
                              uint32_t uxTaskCount,
                              uint32_t ulSubFrameDuration,
                              uint32_t ulTotalSubFrames);

#endif /* TIMELINE_SCHEDULER_H */
