#include "timeline_scheduler.h"
#include "schedule_tasks.h"

/* Auto-generated from schedule.json. DO NOT EDIT. */

const uint32_t g_major_frame_ms = 100u;
const uint32_t g_minor_frame_ms = 10u;
const uint32_t g_subframe_count = 10u;

TimelineTaskConfig_t my_schedule[] = {
    {"Producer", vTaskProducer, HARD_RT, 2u, 5u, 0u, (uint16_t)256u, (uint8_t)0u, NULL, TASK_NOT_STARTED, NULL},
    {"Consumer", vTaskConsumer, HARD_RT, 6u, 9u, 0u, (uint16_t)256u, (uint8_t)1u, NULL, TASK_NOT_STARTED, NULL},
    {"HRT_Mid", vTask1, HARD_RT, 22u, 25u, 2u, (uint16_t)256u, (uint8_t)2u, NULL, TASK_NOT_STARTED, NULL},
    {"SRT_A", vTaskSRT_A, SOFT_RT, 0u, 0u, 0u, (uint16_t)256u, (uint8_t)3u, NULL, TASK_NOT_STARTED, NULL},
    {"SRT_B", vTaskSRT_B, SOFT_RT, 0u, 0u, 0u, (uint16_t)256u, (uint8_t)4u, NULL, TASK_NOT_STARTED, NULL},
};

const uint32_t my_schedule_count = (uint32_t)(sizeof(my_schedule)/sizeof(my_schedule[0]));
