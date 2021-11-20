#ifndef MPY_XT804_MPY_COMMON_H_
#define MPY_XT804_MPY_COMMON_H_

#include "py/mpconfig.h"
#include "py/compile.h"
#include "py/parse.h"

#if MICROPY_ENABLE_COMPILER
void mpy_exec_str(const char *src, mp_parse_input_kind_t input_kind);
#endif

#endif
