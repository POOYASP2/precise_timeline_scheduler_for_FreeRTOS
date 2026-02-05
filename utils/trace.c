#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "trace.h"
#include "uart.h"

static QueueHandle_t xTraceQueue = NULL;

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

    for (int j = 0; j < i; j++) {
        out[j] = tmp[i - 1 - j];
    }
    out[i] = '\0';
}

void vTraceInit(void)
{
    xTraceQueue = xQueueCreate(128, sizeof(TraceLog_t));
}

void TracePush(uint8_t taskId, TraceEvent_t event)
{
    if (xTraceQueue == NULL) return;

    TraceLog_t log;
    log.tick = xTaskGetTickCount();
    log.taskId = taskId;
    log.event = (uint8_t)event;

    xQueueSend(xTraceQueue, &log, 0);
}

void vLoggingTask(void *pvParameters)
{
    (void)pvParameters;

    TraceLog_t r;
    char msg[64];
    char num[16];

    for (;;)
    {
        if (xQueueReceive(xTraceQueue, &r, portMAX_DELAY) == pdPASS)
        {
            u32_to_dec(num, (uint32_t)r.tick);

            int k = 0;
            msg[k++] = '[';
            for (int i = 0; num[i] != '\0' && k < (int)sizeof(msg) - 1; i++) msg[k++] = num[i];
            msg[k++] = ']';
            msg[k++] = ' ';
            msg[k++] = (char)('A' + r.taskId);
            msg[k++] = ' ';

            const char *ev = "";
            if (r.event == TRACE_RELEASE) ev = "RELEASE";
            else if (r.event == TRACE_START) ev = "START";
            else if (r.event == TRACE_COMPLETE) ev = "COMPLETE";
            else if (r.event == TRACE_DEADLINE_MISS) ev = "DEADLINE_MISS";

            for (int i = 0; ev[i] != '\0' && k < (int)sizeof(msg) - 3; i++) msg[k++] = ev[i];

            msg[k++] = '\r';
            msg[k++] = '\n';
            msg[k] = '\0';

            UART_printf(msg);
        }
    }
}
