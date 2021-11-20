

#include "hal_common.h"
#include "libc_port.h"

#include <stdarg.h>

int g_colorful_print = 0;

void hal_print(const char * label, const char * term, const char * fmt, ...) {
    wm_printf(label);
    va_list args;
	va_start(args, fmt);
	wm_vprintf(fmt, args);
	va_end(args);
    wm_printf(term);
}
