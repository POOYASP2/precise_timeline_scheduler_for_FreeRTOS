#include <stdint.h>

// Define the Stack (Reserved memory for function variables)
#define STACK_SIZE 0x400 // 1KB Stack
__attribute__((section(".stack"))) 
static uint32_t stack[STACK_SIZE];

// Forward declaration of the Reset_Handler and Main
void Reset_Handler(void);
extern int main(void);

// Forward declaration of FreeRTOS Interrupt Handlers
// These are defined in the Kernel, we just need to link to them.
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler(void);

// Default Handler for unused interrupts
void Default_Handler(void) {
    while (1);
}

// ================== The Vector Table ==================
// This array is placed at address 0x00 by the linker.
// It tells the CPU where to jump for every event.
void *g_pfnVectors[] __attribute__((section(".isr_vector"))) = {
    (void *)(stack + STACK_SIZE),     // Initial Stack Pointer
    (void *)Reset_Handler,            // Reset Handler (Entry Point)
    (void *)Default_Handler,          // NMI Handler
    (void *)Default_Handler,          // Hard Fault Handler
    (void *)Default_Handler,          // MPU Fault Handler
    (void *)Default_Handler,          // Bus Fault Handler
    (void *)Default_Handler,          // Usage Fault Handler
    0, 0, 0, 0,                       // Reserved
    (void *)vPortSVCHandler,          // SVCall Handler (Used by FreeRTOS to start scheduler)
    (void *)Default_Handler,          // Debug Monitor
    0,                                // Reserved
    (void *)xPortPendSVHandler,       // PendSV Handler (Used by FreeRTOS for context switch)
    (void *)xPortSysTickHandler       // SysTick Handler (Used by FreeRTOS for time base)
};

// The Variables Memory Layout (Provided by Linker Script)
extern uint32_t _etext; // End of code in Flash
extern uint32_t _sdata; // Start of data in RAM
extern uint32_t _edata; // End of data in RAM
extern uint32_t _sbss;  // Start of zero-init data (bss)
extern uint32_t _ebss;  // End of zero-init data (bss)

// ================== The Reset Handler ==================
// This is the first code that runs when the CPU turns on.
void Reset_Handler(void) {
    uint32_t *src, *dst;

    // Enable FPU (Floating Point Unit) 
    volatile uint32_t *CPACR = (uint32_t *) 0xE000ED88;
    *CPACR |= ((3UL << 20) | (3UL << 22));

    // Copy initialized data from Flash to RAM
    src = &_etext;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // Zero out the BSS section (uninitialized variables)
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    // Launch Main
    main();
    
    // If main returns, loop forever
    while (1);
}
