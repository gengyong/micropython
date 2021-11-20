
#ifndef MPY_XT804_LIBC_PORT_H_
#define MPY_XT804_LIBC_PORT_H_

#include <stdarg.h>

// missing declaration in SDK/platform/arch/x804/libc/libc_port.c
int wm_vsnprintf(char* buffer, size_t count, const char* format, va_list va);
int wm_printf(const char *fmt,...);
int wm_vprintf(const char *fmt, va_list arg_ptr);

int sendchar(int ch);
int sendchar1(int ch);

#endif
