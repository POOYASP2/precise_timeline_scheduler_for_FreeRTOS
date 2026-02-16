#ifndef TRACE_H
#define TRACE_H

#include "FreeRTOS.h"
#include "task.h"
#include "timeline_scheduler.h"
#include <stdint.h>

typedef enum
{
    TRACE_RELEASE = 0,
    TRACE_START,
    TRACE_COMPLETE,
    TRACE_DEADLINE_MISS,
    TRACE_IDLE
} TraceEvent_t;

/*
 * Trace record (tick-level):
 *  - tick:      RTOS tick
 *  - frame_ms:  time inside major frame (e.g., ulGlobalTimeInFrame)
 *  - subframe:  current subframe index (e.g., ulCurrentSubFrameIndex)
 *  - taskId:    stable task id (0..N-1), or 0xFF for system events
 *  - event:     TraceEvent_t
 *  - info16:    optional extra (deadline ms, idle ticks, policy code, etc.)
 */
typedef struct
{
    TickType_t tick;
    uint16_t   frame_ms;
    uint8_t    subframe;
    uint8_t    taskId;
    uint8_t    event;
    uint16_t   info16;
} TraceLog_t;

void vTraceInit(void);
uint32_t TraceGetDropped(void);

/* ISR-context push (minimal record) */
void TracePushFromISR(uint8_t taskId,
                      TraceEvent_t event,
                      BaseType_t *pxHigherPriorityTaskWoken);

/* Task-context push (timeline-aware record) */
void TracePushTimeline(uint8_t taskId,
                       TraceEvent_t event,
                       uint16_t frame_ms,
                       uint8_t subframe,
                       uint16_t info16);

/* ISR-context push (timeline-aware record) */
void TracePushTimelineFromISR(uint8_t taskId,
                              TraceEvent_t event,
                              uint16_t frame_ms,
                              uint8_t subframe,
                              uint16_t info16,
                              BaseType_t *pxHigherPriorityTaskWoken);

/* Helper for idle time (taskId fixed to 0xFF) */
void TracePushIdle(uint16_t idle_ticks,
                   uint16_t frame_ms,
                   uint8_t subframe);

void vLoggingTask(void *pvParameters);

void TraceRegisterTaskName(uint8_t taskId, const char *name);

void vTraceRegisterNamesFromSchedule(const TimelineTaskConfig_t *schedule,
                                            uint32_t numTasks);

#endif /* TRACE_H */
