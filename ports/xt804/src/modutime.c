/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "py/runtime.h"
#include "shared/timeutils/timeutils.h"
#include "extmod/utime_mphal.h"
#include "hal_common.h"
#include "mphalport.h"

STATIC mp_obj_t mp_utime_localtime(size_t n_args, const mp_obj_t *args) {
    timeutils_struct_time_t tm;
    if (n_args == 0 || args[0] == mp_const_none) {
        RTC_TimeTypeDef rtctime;
        HAL_PMU_RTC_GetTime(&xt804_rtc_source, &rtctime);
        timeutils_rtc_to_struct_time(&rtctime, &tm);
    } else {
        mp_int_t seconds = mp_obj_get_int(args[0]);
        timeutils_seconds_since_epoch_to_struct_time(seconds, &tm);
    }
    mp_obj_t tuple[8] = {
        tuple[0] = mp_obj_new_int(tm.tm_year),
        tuple[1] = mp_obj_new_int(tm.tm_mon),
        tuple[2] = mp_obj_new_int(tm.tm_mday),
        tuple[3] = mp_obj_new_int(tm.tm_hour),
        tuple[4] = mp_obj_new_int(tm.tm_min),
        tuple[5] = mp_obj_new_int(tm.tm_sec),
        tuple[6] = mp_obj_new_int(tm.tm_wday),
        tuple[7] = mp_obj_new_int(tm.tm_yday),
    };
    return mp_obj_new_tuple(8, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_utime_localtime_obj, 0, 1, mp_utime_localtime);

STATIC mp_obj_t mp_utime_mktime(mp_obj_t tuple) {
    size_t len;
    mp_obj_t *elem;
    mp_obj_get_array(tuple, &len, &elem);

    // localtime generates a tuple of len 8. CPython uses 9, so we accept both.
    if (len < 8 || len > 9) {
        mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("mktime needs a tuple of length 8 or 9 (%d given)"), len);
    }

    return mp_obj_new_int_from_uint(timeutils_mktime(mp_obj_get_int(elem[0]),
        mp_obj_get_int(elem[1]), mp_obj_get_int(elem[2]), mp_obj_get_int(elem[3]),
        mp_obj_get_int(elem[4]), mp_obj_get_int(elem[5])));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_utime_mktime_obj, mp_utime_mktime);

STATIC mp_obj_t mp_utime_time(void) {
    uint64_t sec = mp_hal_time_s();
    return mp_obj_new_int(sec);
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_utime_time_obj, mp_utime_time);

STATIC mp_obj_t mp_utime_unixtime(void) {
    uint64_t sec = mp_hal_time_s() + TIMEUTILS_SECONDS_1970_TO_2000;
    return mp_obj_new_int(sec);
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_utime_unixtime_obj, mp_utime_unixtime);

STATIC const mp_rom_map_elem_t mp_module_utime_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_time) },

    { MP_ROM_QSTR(MP_QSTR_gmtime), MP_ROM_PTR(&mp_utime_localtime_obj) },
    { MP_ROM_QSTR(MP_QSTR_localtime), MP_ROM_PTR(&mp_utime_localtime_obj) },
    { MP_ROM_QSTR(MP_QSTR_mktime), MP_ROM_PTR(&mp_utime_mktime_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&mp_utime_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_unixtime), MP_ROM_PTR(&mp_utime_unixtime_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&mp_utime_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_ms), MP_ROM_PTR(&mp_utime_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_us), MP_ROM_PTR(&mp_utime_sleep_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_ms), MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_us), MP_ROM_PTR(&mp_utime_ticks_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_cpu), MP_ROM_PTR(&mp_utime_ticks_cpu_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_add), MP_ROM_PTR(&mp_utime_ticks_add_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_diff), MP_ROM_PTR(&mp_utime_ticks_diff_obj) },
    { MP_ROM_QSTR(MP_QSTR_time_ns), MP_ROM_PTR(&mp_utime_time_ns_obj) },

    { MP_ROM_QSTR(MP_QSTR_SECONDS_1970_TO_2000), MP_ROM_INT(TIMEUTILS_SECONDS_1970_TO_2000) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_utime_globals, mp_module_utime_globals_table);

const mp_obj_module_t mp_module_utime = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_utime_globals,
};
