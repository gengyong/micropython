/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Damien P. George
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

#include "py/runtime.h"
#include "py/mperrno.h"

#include "modmachine.h"

#include <wm_wdg.h>

typedef struct _machine_wdt_obj_t {
    mp_obj_base_t base;
} machine_wdt_obj_t;

STATIC const machine_wdt_obj_t machine_wdt = {{&machine_wdt_type}};

STATIC mp_obj_t machine_wdt_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 5000} },
    };

    // Parse the arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Verify the WDT id.
    mp_int_t id = args[ARG_id].u_int;
    if (id != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("WDT(%d) doesn't exist"), id);
    }

    mp_int_t timeout = args[ARG_timeout].u_int;
    if (timeout < 0) {
         mp_raise_ValueError(MP_ERROR_TEXT("param `timeout' must be positive integer."));
    }

    // Start the watchdog (timeout is in milliseconds).
    //watchdog_enable(args[ARG_timeout].u_int, false);
    WDG_HandleTypeDef wdg;
    wdg.Instance = WDG;
    wdg.Init.Reload = timeout * 1000;
    if (HAL_OK == HAL_WDG_Init(&wdg)) {
        TLOG("WDT started.");
        HAL_NVIC_SetPriority(WDG_IRQn, 0);
        HAL_NVIC_EnableIRQ(WDG_IRQn);
    } else {
        mp_raise_OSError(MP_EIO);
    }

    return MP_OBJ_FROM_PTR(&machine_wdt);
}

STATIC mp_obj_t machine_wdt_feed(mp_obj_t self_in) {
    WDG->CLR = WDG_CLR;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_feed_obj, machine_wdt_feed);

STATIC mp_obj_t machine_wdt_cancel(mp_obj_t self_in) {
    WDG_HandleTypeDef wdg;
    wdg.Instance = WDG;
    HAL_WDG_DeInit(&wdg);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_cancel_obj, machine_wdt_cancel);

STATIC mp_obj_t machine_wdt_left(mp_obj_t self_in) {
    uint32_t left = READ_REG(WDG->VAL);
    return MP_OBJ_NEW_SMALL_INT(left/1000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_left_obj, machine_wdt_left);

STATIC mp_obj_t machine_wdt_past(mp_obj_t self_in) {
    mp_int_t past = READ_REG(WDG->LD) - READ_REG(WDG->VAL);
    return MP_OBJ_NEW_SMALL_INT(MAX(past/1000, 0));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_past_obj, machine_wdt_past);

STATIC const mp_rom_map_elem_t machine_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&machine_wdt_feed_obj) },
    { MP_ROM_QSTR(MP_QSTR_cancel), MP_ROM_PTR(&machine_wdt_cancel_obj) },
    { MP_ROM_QSTR(MP_QSTR_left), MP_ROM_PTR(&machine_wdt_left_obj) },
    { MP_ROM_QSTR(MP_QSTR_past), MP_ROM_PTR(&machine_wdt_past_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_wdt_locals_dict, machine_wdt_locals_dict_table);

const mp_obj_type_t machine_wdt_type = {
    { &mp_type_type },
    .name = MP_QSTR_WDT,
    .make_new = machine_wdt_make_new,
    .locals_dict = (mp_obj_dict_t *)&machine_wdt_locals_dict,
};
