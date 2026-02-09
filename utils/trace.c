/* trace.c */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "trace.h"
#include "uart.h"
#include <stdint.h>

#ifndef TRACE_QUEUE_LEN
#define TRACE_QUEUE_LEN 256
#endif

static QueueHandle_t xTraceQueue = NULL;
static volatile uint32_t ulDropped = 0;

static void u32_to_dec(char *out, uint32_t v)
{
    char tmp[11];
    int i = 0;

    if (v == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    while (v > 0 && i < 10) {
        tmp[i++] = (char)('0' + (v % 10u));
        v /= 10u;
    }

    for (int j = 0; j < i; j++) out[j] = tmp[i - 1 - j];
    out[i] = '\0';
}

static void u16_to_dec(char *out, uint16_t v)
{
    char tmp[6];
    int i = 0;

    if (v == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    while (v > 0 && i < 5) {
        tmp[i++] = (char)('0' + (v % 10u));
        v /= 10u;
    }

    for (int j = 0; j < i; j++) out[j] = tmp[i - 1 - j];
    out[i] = '\0';
}

void vTraceInit(void)
{
    xTraceQueue = xQueueCreate(TRACE_QUEUE_LEN, sizeof(TraceLog_t));
    ulDropped = 0;
}

uint32_t TraceGetDropped(void)
{
    return ulDropped;
}

/* Compatibility API: task-context minimal record */
void TracePush(uint8_t taskId, TraceEvent_t event)
{
    if (xTraceQueue == NULL) return;

    TraceLog_t log;
    log.tick     = xTaskGetTickCount();
    log.frame_ms = 0;
    log.subframe = 0;
    log.taskId   = taskId;
    log.event    = (uint8_t)event;
    log.info16   = 0;

    if (xQueueSend(xTraceQueue, &log, 0) != pdPASS) {
        ulDropped++;
    }
}

/* Compatibility API: ISR-context minimal record */
void TracePushFromISR(uint8_t taskId,
                      TraceEvent_t event,
                      BaseType_t *pxHigherPriorityTaskWoken)
{
    if (xTraceQueue == NULL) return;

    TraceLog_t log;
    log.tick     = xTaskGetTickCountFromISR();
    log.frame_ms = 0;
    log.subframe = 0;
    log.taskId   = taskId;
    log.event    = (uint8_t)event;
    log.info16   = 0;

    if (xQueueSendFromISR(xTraceQueue, &log, pxHigherPriorityTaskWoken) != pdPASS) {
        ulDropped++;
    }
}

/* Task-context timeline-aware record */
void TracePushTimeline(uint8_t taskId,
                       TraceEvent_t event,
                       uint16_t frame_ms,
                       uint8_t subframe,
                       uint16_t info16)
{
    if (xTraceQueue == NULL) return;

    TraceLog_t log;
    log.tick     = xTaskGetTickCount();
    log.frame_ms = frame_ms;
    log.subframe = subframe;
    log.taskId   = taskId;
    log.event    = (uint8_t)event;
    log.info16   = info16;

    if (xQueueSend(xTraceQueue, &log, 0) != pdPASS) {
        ulDropped++;
    }
}

/* ISR-context timeline-aware record */
void TracePushTimelineFromISR(uint8_t taskId,
                              TraceEvent_t event,
                              uint16_t frame_ms,
                              uint8_t subframe,
                              uint16_t info16,
                              BaseType_t *pxHigherPriorityTaskWoken)
{
    if (xTraceQueue == NULL) return;

    TraceLog_t log;
    log.tick     = xTaskGetTickCountFromISR();
    log.frame_ms = frame_ms;
    log.subframe = subframe;
    log.taskId   = taskId;
    log.event    = (uint8_t)event;
    log.info16   = info16;

    if (xQueueSendFromISR(xTraceQueue, &log, pxHigherPriorityTaskWoken) != pdPASS) {
        ulDropped++;
    }
}

/* System helper: IDLE accounting */
void TracePushIdle(uint16_t idle_ticks,
                   uint16_t frame_ms,
                   uint8_t subframe)
{
    /* system events use taskId = 0xFF */
    TracePushTimeline(0xFFu, TRACE_IDLE, frame_ms, subframe, idle_ticks);
}

void vLoggingTask(void *pvParameters)
{
    (void)pvParameters;

    TraceLog_t r;
    char msg[112];
    char num32[16];
    char num16_a[8];
    char num16_b[8];

    for (;;)
    {
        if (xQueueReceive(xTraceQueue, &r, portMAX_DELAY) == pdPASS)
        {
            u32_to_dec(num32, (uint32_t)r.tick);
            u16_to_dec(num16_a, r.frame_ms);
            u16_to_dec(num16_b, r.info16);

            const char *ev = "";
            if      (r.event == TRACE_RELEASE)       ev = "RELEASE";
            else if (r.event == TRACE_START)         ev = "START";
            else if (r.event == TRACE_COMPLETE)      ev = "COMPLETE";
            else if (r.event == TRACE_DEADLINE_MISS) ev = "DEADLINE_MISS";
            else if (r.event == TRACE_IDLE)          ev = "IDLE";

            int k = 0;

            /* [tick] */
            msg[k++] = '[';
            for (int i = 0; num32[i] != '\0' && k < (int)sizeof(msg) - 1; i++) msg[k++] = num32[i];
            msg[k++] = ']';
            msg[k++] = ' ';

            /* Optional timeline fields if provided */
            if (r.frame_ms != 0 || r.subframe != 0) {
                msg[k++] = 's'; msg[k++] = 'f'; msg[k++] = '=';
                msg[k++] = (char)('0' + (r.subframe % 10u));
                msg[k++] = ' ';
                msg[k++] = 't'; msg[k++] = '=';
                for (int i = 0; num16_a[i] != '\0' && k < (int)sizeof(msg) - 1; i++) msg[k++] = num16_a[i];
                msg[k++] = 'm'; msg[k++] = 's';
                msg[k++] = ' ';
            }

            /* Task label */
            if (r.taskId == 0xFFu) {
                msg[k++] = 'S'; msg[k++] = 'Y'; msg[k++] = 'S';
            } else {
                msg[k++] = (char)('A' + r.taskId);
            }
            msg[k++] = ' ';

            /* Event text */
            for (int i = 0; ev[i] != '\0' && k < (int)sizeof(msg) - 1; i++) msg[k++] = ev[i];

            /* Optional info16 */
            if (r.info16 != 0) {
                msg[k++] = ' ';
                msg[k++] = '(';
                msg[k++] = 'i'; msg[k++] = 'n'; msg[k++] = 'f'; msg[k++] = 'o'; msg[k++] = '=';
                for (int i = 0; num16_b[i] != '\0' && k < (int)sizeof(msg) - 1; i++) msg[k++] = num16_b[i];
                msg[k++] = ')';
            }

            msg[k++] = '\r';
            msg[k++] = '\n';
            msg[k] = '\0';

            UART_printf(msg);
        }
    }
}
