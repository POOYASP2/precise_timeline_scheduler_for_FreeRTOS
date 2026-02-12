#include <stdint.h>

void Reset_Handler(void);
extern int main(void);

/* FreeRTOS handlers (provided by port.c for ARM_CM4F) */
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

void Default_Handler(void)
{
    while (1)
    {
    }
}

/* Linker symbols */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata, _edata;
extern uint32_t _sbss, _ebss;

__attribute__((section(".isr_vector"))) void *g_pfnVectors[] = {
    (void *)&_estack,           /* Initial MSP */
    (void *)Reset_Handler,      /* Reset */
    (void *)Default_Handler,    /* NMI */
    (void *)Default_Handler,    /* HardFault */
    (void *)Default_Handler,    /* MemManage */
    (void *)Default_Handler,    /* BusFault */
    (void *)Default_Handler,    /* UsageFault */
    0, 0, 0, 0,                 /* Reserved */
    (void *)vPortSVCHandler,    /* SVC */
    (void *)Default_Handler,    /* DebugMon */
    0,                          /* Reserved */
    (void *)xPortPendSVHandler, /* PendSV */
    (void *)xPortSysTickHandler /* SysTick */
};

void Reset_Handler(void)
{
    uint32_t *src, *dst;

    /* Copy .data from FLASH to RAM */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* Zero .bss */
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0;
    }

    (void)main();

    while (1)
    {
    }
}
