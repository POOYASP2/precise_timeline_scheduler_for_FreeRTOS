#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART0_BASE      (0x40004000UL)
#define UART0_DATA      (*((volatile uint32_t *)(UART0_BASE + 0x00)))
#define UART0_STATE     (*((volatile uint32_t *)(UART0_BASE + 0x04)))
#define UART0_CTRL      (*((volatile uint32_t *)(UART0_BASE + 0x08)))
#define UART0_BAUDDIV   (*((volatile uint32_t *)(UART0_BASE + 0x10)))

void UART_init(void);
void UART_printf(const char *s);

#endif
