

#include "hal_common.h"
#include "libc_port.h"

#include <stdarg.h>

void hal_log(const char * fmt, ...) {
    wm_printf("\r\n\x1b[97;42m[HAL]\x1B[0m\x1b[32m ");
    va_list args;
	va_start(args, fmt);
	wm_vprintf(fmt, args);    
	va_end(args);
    wm_printf("\x1b[0m\r\n");
}

void hal_warn(const char * fmt, ...) {
    wm_printf("\r\n\x1b[30;43m[HAL]\x1B[0m\x1b[33m[WARN] ");
    va_list args;
	va_start(args, fmt);
	wm_vprintf(fmt, args);
	va_end(args);
    wm_printf("\x1b[0m\r\n");
}

void hal_error(const char * fmt, ...) {
    wm_printf("\r\n\x1b[97;41m[HAL]\x1B[0m\x1b[31m[ERROR] ");
    va_list args;
	va_start(args, fmt);
	wm_vprintf(fmt, args);
	va_end(args);
    wm_printf("\x1b[0m\r\n");
}

void hal_panic(const char * fmt, ...) {
    wm_printf("\r\n\x1b[5m\x1b[30;101m[HAL][PANIC] ");
    va_list args;
	va_start(args, fmt);
	wm_vprintf(fmt, args);
	va_end(args);
    wm_printf("\x1b[0m\r\n");
}