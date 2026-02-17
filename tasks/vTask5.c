#include "generated/schedule_tasks.h"

void vTask5(void *pvParams)
{
    for (int i = 0; i < (50000000)*2; i++)
    {
        __asm volatile("nop");
    }
}