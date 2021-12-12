

#include "hal_common.h"
#include "py/mpconfig.h"
#include "py/obj.h"


void nlr_jump_fail(void *val) {
    TPANIC( "uncaught exception %p\n", val)
    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(val));
}

void __fatal_error(const char *msg) {
    TPANIC(msg);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    TERROR("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
}
#endif


