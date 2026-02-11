#include "uart.h"

void UART_init(void)
{
    UART0_BAUDDIV = 16;
    UART0_CTRL = 1;
}

void UART_printf(const char *s)
{
    while (*s != '\0') {
        UART0_DATA = (unsigned int)(*s);
        s++;
    }
}
