

#ifndef HEADER_HAK_INIT_H
#define HEADER_HAK_INIT_H

void Error_Handler(void);
void HAL_UART0_Init();

// missing declaration in SDK/platform/arch/x804/libc/libc_port.c
int sendchar(int ch);
int sendchar1(int ch);

#endif
