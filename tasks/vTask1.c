#include "generated/schedule_tasks.h"

void vTask1(void *pvParams)
{
    for (int i = 0; i < (500)*2; i++)
    {
        __asm volatile("nop");
    }
}