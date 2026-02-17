#ifndef SCHEDULE_TASKS_H
#define SCHEDULE_TASKS_H

#include "FreeRTOS.h"
#include "task.h"

/* Prototypes for tasks referenced by generated schedule_config.c */
void vTask1(void *pvParams);
void vTask2(void *pvParams);
void vTask3(void *pvParams);

void vTaskSRT_A(void *pvParams);
void vTaskSRT_B(void *pvParams);

void vTaskProducer(void *pvParams);
void vTaskConsumer(void *pvParams);

#endif
