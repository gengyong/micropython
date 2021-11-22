/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Geng Yong
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

#include "wm_gpio.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/virtpin.h"
#include "modmachine.h"
#include "machine_pin.h"
#include "hal_common.h"

#define INVALID_PIN_ID ((uint32_t)(-1))
#define GPIO_FEATURE_NONE


STATIC const machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type}, GPIOA, GPIO_PIN_0,  {'A', '0', 0, 0}, 0, 0},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_1,  {'A', '1', 0, 0}, 0, 1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_2,  {'A', '2', 0, 0}, 0, 2},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_3,  {'A', '3', 0, 0}, 0, 3},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_4,  {'A', '4', 0, 0}, 0, 4},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_5,  {'A', '5', 0, 0}, 0, 5},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_6,  {'A', '6', 0, 0}, 0, 6},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_7,  {'A', '7', 0, 0}, 0, 7},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_8,  {'A', '8', 0, 0}, 0, 8},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_9,  {'A', '9', 0, 0}, 0, 9},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_10, {'A', '1', '0', 0}, 0, 10},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_11, {'A', '1', '1', 0}, 0, 11},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_12, {'A', '1', '2', 0}, 0, 12},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_13, {'A', '1', '3', 0}, 0, 13},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_14, {'A', '1', '4', 0},  0, 14},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_15, {'A', '1', '5', 0}, 0, 15},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_0,  {'B', '0', 0, 0}, 0, 32},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_1,  {'B', '1', 0, 0}, 0, 33},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_2,  {'B', '2', 0, 0}, 0, 34},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_3,  {'B', '3', 0, 0}, 0, 35},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_4,  {'B', '4', 0, 0}, 0, 36},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_5,  {'B', '5', 0, 0}, 0, 37},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_6,  {'B', '6', 0, 0}, 0, 38},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_7,  {'B', '7', 0, 0}, 0, 39},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_8,  {'B', '8', 0, 0}, 0, 40},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_9,  {'B', '9', 0, 0}, 0, 41},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_10, {'B', '1', '0', 0}, 0, 42},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_11, {'B', '1', '1', 0}, 0, 43},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_12, {'B', '1', '2', 0}, 0, 44},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_13, {'B', '1', '3', 0}, 0, 45},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_14, {'B', '1', '4', 0}, 0, 46},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_15, {'B', '1', '5', 0}, 0, 47},
};

// forward declaration
STATIC const machine_pin_irq_obj_t machine_pin_irq_object[];

void machine_pins_init(void) {
    static bool did_install = false;
    if (!did_install) {
        //gpio_install_isr_service(0);
        did_install = true;
    }
    memset(&MP_STATE_PORT(machine_pin_irq_handler[0]), 0, sizeof(MP_STATE_PORT(machine_pin_irq_handler)));
}

void machine_pins_deinit(void) {
    for (int i = 0; i < MP_ARRAY_SIZE(machine_pin_obj); ++i) {
        if (machine_pin_obj[i].pin != (uint32_t)(-1)) {
            //gpio_isr_handler_remove(machine_pin_obj[i].id);
        }
    }
}

STATIC void machine_pin_isr_handler(void *arg) {
    // machine_pin_obj_t *self = arg;
    // mp_obj_t handler = MP_STATE_PORT(machine_pin_irq_handler)[self->id];
    // mp_sched_schedule(handler, MP_OBJ_FROM_PTR(self));
    // mp_hal_wake_main_task_from_isr();
}


uint32_t machine_pin_get_id(mp_obj_t pin_in) {
    const mp_obj_type_t * obj_type = mp_obj_get_type(pin_in);
    if (obj_type != &machine_pin_type) {
        if (obj_type == &mp_type_str) {
            const char * nm = mp_obj_str_get_str(pin_in);
            char prefix = *nm;
            int baseIndex = 0;
            if (prefix >= 'a' && prefix <= 'z') {
                baseIndex = (prefix - 'a') * 32;
            } else if (prefix >= 'A' && prefix <= 'Z') {
                baseIndex = (prefix - 'A') * 32;
            } else if (prefix >= '0' && prefix <= '9') {
                int id = 0;
                const char * p = nm;
                for (; ((*p) >= '0' && (*p) <= '9'); p++) {
                    id *= 10;
                    id += (*p) - '0';
                }
                if (*p != 0) {
                    // if has invalid char in pin num.
                    mp_raise_ValueError(MP_ERROR_TEXT("expecting a pin"));
                    return INVALID_PIN_ID;
                }
                return id;
            } else {
                mp_raise_ValueError(MP_ERROR_TEXT("expecting a pin"));
                return INVALID_PIN_ID;
            }
            int num = 0;
            for (const char * p = nm + 1; ((*p) >= '0' && (*p) <= '9' && num < 32); p++) {
                num *= 10;
                num += (*p) - '0';
            }
            if (num < 32) {
                return baseIndex + num;
            }
            mp_raise_ValueError(MP_ERROR_TEXT("pin index out of range"));
            return INVALID_PIN_ID;
        } else if (obj_type == &mp_type_int) {
            return mp_obj_get_int(pin_in);
        }
        mp_raise_ValueError(MP_ERROR_TEXT("expecting a pin"));
        return INVALID_PIN_ID;
    }
    machine_pin_obj_t *self = pin_in;
    return self->id;
}

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = self_in;
    char prefix = (self->id / 32) + 'A';
    char pinnum = (self->id % 32);
    mp_printf(print, "Pin('%c%d')", prefix, pinnum);
}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(-1)}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    };

    TDEBUG("machine_pin_obj_init_helper: pin(%u) found", self->id);

    if (self->id == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PIN 'PA0' unavialiable."));
        return mp_const_none;
    }

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_mode].u_obj == mp_const_none) {
        mp_raise_ValueError(MP_ERROR_TEXT("PIN 'mode' not set."));
        return mp_const_none;
    }

    // reset the pin to digital if this is a mode-setting init (grab it back from ADC)
    if (args[ARG_mode].u_obj != mp_const_none) {
        TDEBUG("machine_pin_obj_init_helper: pin(%u) HAL_GPIO_DeInit()", self->id);
        HAL_GPIO_DeInit(self->gpio, self->pin);
    }
    
    GPIO_InitTypeDef initDef;

    initDef.Pin = self->pin;
    initDef.Mode = mp_obj_get_int(args[ARG_mode].u_obj);
    if (args[ARG_pull].u_obj == mp_const_none) {
        initDef.Pull = GPIO_NOPULL;
    } else {
        initDef.Pull = mp_obj_get_int(args[ARG_pull].u_obj);
    }

    TDEBUG("machine_pin_obj_init_helper: pin(%u) HAL_GPIO_Init(mode:%u, pull:%u)", 
        self->id, initDef.Mode, initDef.Pull);

    HAL_GPIO_Init(self->gpio, &initDef);

    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
         if (initDef.Mode == GPIO_MODE_OUTPUT) {
            TDEBUG("machine_pin_obj_init_helper: pin(%u) set default value:  HAL_GPIO_WritePin(%u)", 
                self->id, mp_obj_is_true(args[ARG_value].u_obj) ? 1 : 0);

            HAL_GPIO_WritePin(self->gpio, self->pin, 
                mp_obj_is_true(args[ARG_value].u_obj) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
    }

    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    TDEBUG("mp_pin_make_new");
    // get the wanted pin object
    uint32_t pin_id = machine_pin_get_id(args[0]);
    if (pin_id == INVALID_PIN_ID) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }
    TDEBUG("mp_pin_make_new: pin id: %u", pin_id);

    const machine_pin_obj_t *self = NULL;
    for (int i = 0; i < MP_ARRAY_SIZE(machine_pin_obj); i++) {
        if (machine_pin_obj[i].id == pin_id) {
            self = (machine_pin_obj_t *)&machine_pin_obj[i];
            break;
        }
    }
    
    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }

    TDEBUG("mp_pin_make_new: pin(%u) found", pin_id);

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = self_in;
    if (n_args == 0) {
        // get pin
        TDEBUG("machine_pin_call: read pin(%u)", self->id);
        GPIO_PinState state = HAL_GPIO_ReadPin(self->gpio, self->pin);
        return MP_OBJ_NEW_SMALL_INT(state == GPIO_PIN_SET ? 1:0);
    } else {
        // set pin
        TDEBUG("machine_pin_call: set pin(%u) = %d", self->id,  mp_obj_is_true(args[0]) ? 1:0);
        GPIO_PinState state = mp_obj_is_true(args[0]) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(self->gpio, self->pin, state);
        return mp_const_none;
    }
}

// pin.init(mode, pull)
STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

// pin.low()
STATIC mp_obj_t machine_pin_low(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    HAL_GPIO_WritePin(self->gpio, self->pin, GPIO_PIN_RESET);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_low_obj, machine_pin_low);

// pin.high()
STATIC mp_obj_t machine_pin_high(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    HAL_GPIO_WritePin(self->gpio, self->pin, GPIO_PIN_SET);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_high_obj, machine_pin_high);

// pin.toggle()
STATIC mp_obj_t machine_pin_toggle(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    HAL_GPIO_TogglePin(self->gpio, self->pin);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_toggle_obj, machine_pin_toggle);


// pin.irq(handler=None, trigger=IRQ_FALLING|IRQ_RISING)
STATIC mp_obj_t machine_pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    /*
    enum { ARG_handler, ARG_trigger, ARG_wake };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE} },
        { MP_QSTR_wake, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (n_args > 1 || kw_args->used != 0) {
        // configure irq
        mp_obj_t handler = args[ARG_handler].u_obj;
        uint32_t trigger = args[ARG_trigger].u_int;
        mp_obj_t wake_obj = args[ARG_wake].u_obj;

        if ((trigger == GPIO_PIN_INTR_LOLEVEL || trigger == GPIO_PIN_INTR_HILEVEL) && wake_obj != mp_const_none) {
            mp_int_t wake;
            if (mp_obj_get_int_maybe(wake_obj, &wake)) {
                if (wake < 2 || wake > 7) {
                    mp_raise_ValueError(MP_ERROR_TEXT("bad wake value"));
                }
            } else {
                mp_raise_ValueError(MP_ERROR_TEXT("bad wake value"));
            }

            if (machine_rtc_config.wake_on_touch) { // not compatible
                mp_raise_ValueError(MP_ERROR_TEXT("no resources"));
            }

            if (!RTC_IS_VALID_EXT_PIN(self->id)) {
                mp_raise_ValueError(MP_ERROR_TEXT("invalid pin for wake"));
            }

            if (machine_rtc_config.ext0_pin == -1) {
                machine_rtc_config.ext0_pin = self->id;
            } else if (machine_rtc_config.ext0_pin != self->id) {
                mp_raise_ValueError(MP_ERROR_TEXT("no resources"));
            }

            machine_rtc_config.ext0_level = trigger == GPIO_PIN_INTR_LOLEVEL ? 0 : 1;
            machine_rtc_config.ext0_wake_types = wake;
        } else {
            if (machine_rtc_config.ext0_pin == self->id) {
                machine_rtc_config.ext0_pin = -1;
            }

            if (handler == mp_const_none) {
                handler = MP_OBJ_NULL;
                trigger = 0;
            }
            gpio_isr_handler_remove(self->id);
            MP_STATE_PORT(machine_pin_irq_handler)[self->id] = handler;
            gpio_set_intr_type(self->id, trigger);
            gpio_isr_handler_add(self->id, machine_pin_isr_handler, (void *)self);
        }
    }
    */
    // return the irq object
    //return MP_OBJ_FROM_PTR(&machine_pin_irq_object[self->id]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_irq_obj, 1, machine_pin_irq);

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_low), MP_ROM_PTR(&machine_pin_low_obj) },
    { MP_ROM_QSTR(MP_QSTR_high), MP_ROM_PTR(&machine_pin_high_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&machine_pin_low_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&machine_pin_high_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&machine_pin_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_pin_irq_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(GPIO_MODE_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(GPIO_MODE_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(GPIO_PULLUP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(GPIO_PULLDOWN) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(GPIO_NOPULL) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(6) }, //?
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(7) },    //?
};

STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    
    (void)errcode;
    machine_pin_obj_t *self = self_in;

    switch (request) {
        case MP_PIN_READ: {
            TDEBUG("pin_ioctl: read pin(%u)", self->id);
            return HAL_GPIO_ReadPin(self->gpio, self->pin) == GPIO_PIN_SET ? 1:0;
        }
        case MP_PIN_WRITE: {
            TDEBUG("pin_ioctl: write pin(%u) = %u", self->id, arg);
            HAL_GPIO_WritePin(self->gpio, self->pin, arg ? GPIO_PIN_SET : GPIO_PIN_RESET);
            return 0;
        }
    }

    return -1;
}

STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

STATIC const mp_pin_p_t pin_pin_p = {
    .ioctl = pin_ioctl,
};

const mp_obj_type_t machine_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = machine_pin_print,
    .make_new = mp_pin_make_new,
    .call = machine_pin_call,
    .protocol = &pin_pin_p,
    .locals_dict = (mp_obj_t)&machine_pin_locals_dict,
};

/******************************************************************************/
// Pin IRQ object

STATIC const mp_obj_type_t machine_pin_irq_type;

STATIC const machine_pin_irq_obj_t machine_pin_irq_object[] = {
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_0, 0, 0},
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_1, 0, 1},
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_2, 0, 2},
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_3, 0, 3},
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_4, 0, 4},
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_5, 0, 5},
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_6, 0, 6},
    {{&machine_pin_irq_type}, GPIOA, GPIO_PIN_7, 0, 7},

    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_0, 0, 32},
    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_1, 0, 33},
    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_2, 0, 34},
    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_3, 0, 35},
    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_4, 0, 36},
    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_5, 0, 37},
    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_6, 0, 38},
    {{&machine_pin_irq_type}, GPIOB, GPIO_PIN_7, 0, 39},
};

STATIC mp_obj_t machine_pin_irq_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = self_in;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    machine_pin_isr_handler((void *)&machine_pin_obj[self->id]);
    return mp_const_none;
}

STATIC mp_obj_t machine_pin_irq_trigger(size_t n_args, const mp_obj_t *args) {
    // machine_pin_irq_obj_t *self = args[0];
    // uint32_t orig_trig = GPIO.pin[self->id].int_type;
    // if (n_args == 2) {
    //      // set trigger
    //     gpio_set_intr_type(self->id, mp_obj_get_int(args[1]));
    // }
    // // return original trigger value
    // return MP_OBJ_NEW_SMALL_INT(orig_trig);
     return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_irq_trigger_obj, 1, 2, machine_pin_irq_trigger);

STATIC const mp_rom_map_elem_t machine_pin_irq_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_trigger), MP_ROM_PTR(&machine_pin_irq_trigger_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_irq_locals_dict, machine_pin_irq_locals_dict_table);

STATIC const mp_obj_type_t machine_pin_irq_type = {
    { &mp_type_type },
    .name = MP_QSTR_IRQ,
    .call = machine_pin_irq_call,
    .locals_dict = (mp_obj_dict_t *)&machine_pin_irq_locals_dict,
};
