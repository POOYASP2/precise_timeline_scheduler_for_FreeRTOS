#ifndef TRACE_H
#define TRACE_H

#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

typedef enum
{
    TRACE_RELEASE = 0,
    TRACE_START,
    TRACE_COMPLETE,
    TRACE_DEADLINE_MISS
} TraceEvent_t;

typedef struct
{
    TickType_t   tick;
    uint8_t      taskId;
    uint8_t      event;
} TraceLog_t;

void vTraceInit(void);
void TracePush(uint8_t taskId, TraceEvent_t event);
void vLoggingTask(void *pvParameters);

#endif
