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

#include <wm_gpio.h>
#include <wm_type_def.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/mpirq.h"
#include "extmod/virtpin.h"
#include "modmachine.h"
#include "machine_pin.h"
#include "hal_common.h"

#include "mphalport.h"

#define INVALID_PIN_ID ((uint32_t)(-1))

static inline uint32_t pin_str_to_pin_id(const char * nm) {
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
            return INVALID_PIN_ID;
        }
        return id;
    } else {
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
    return INVALID_PIN_ID;
}

//-----------------------------------------
typedef struct _machine_pin_irq_obj_t {
    mp_irq_obj_t base;
    uint32_t flags;
    uint32_t trigger;
    uint8_t  enabled;
} machine_pin_irq_obj_t;


typedef mp_uint_t (*mp_irq_switch_fun_t)(mp_obj_t self);
typedef struct _machine_pin_irq_methods_t {
    mp_irq_methods_t base;
    mp_irq_switch_fun_t disable;
    mp_irq_switch_fun_t enable;
} machine_pin_irq_methods_t;

STATIC const machine_pin_irq_methods_t machine_pin_irq_methods;

// bootmode Pin: PWM2, I2S_MCLK, LSPI_CS, I2S_DO
STATIC const machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type}, GPIOA, GPIO_PIN_0, PIN_FEATURE_GPIO | PIN_FEATURE_SPI_CS, {'A', '0', 0, 0 }, 0, 0},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_1, PIN_FEATURE_GPIO | PIN_FEATURE_PWN3 | PIN_FEATURE_ADC0 , {'A', '1', 0, 0 }, 1, 1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_2, PIN_FEATURE_GPIO | PIN_FEATURE_PWN0 | PIN_FEATURE_ADC3 , {'A', '2', 0, 0 }, 2, 2},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_3, PIN_FEATURE_GPIO | PIN_FEATURE_PWN1 | PIN_FEATURE_ADC2, {'A', '3', 0, 0 }, 3, 3},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_4, PIN_FEATURE_GPIO | PIN_FEATURE_PWN4 | PIN_FEATURE_ADC1, {'A', '4', 0, 0 }, 4, 4},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_5, PIN_FEATURE_GPIO | PIN_FEATURE_PWNBREAK, {'A', '5', 0, 0 }, 5, 5},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_6, PIN_FEATURE_GPIO, {'A', '6', 0, 0 }, 6, 6},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_7, PIN_FEATURE_GPIO | PIN_FEATURE_SPI_MOSI, {'A', '7', 0, 0 }, 7, 7},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_8, PIN_FEATURE_GPIO, {'A', '8', 0, 0 }, 8, 8},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_9, PIN_FEATURE_GPIO, {'A', '9', 0, 0 }, 9, 9},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_10, PIN_FEATURE_GPIO | PIN_FEATURE_PWN0, {'A', '1', '0', 0 }, 10, 10},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_11, PIN_FEATURE_GPIO | PIN_FEATURE_PWN1, {'A', '1', '1', 0 }, 11, 11},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_12, PIN_FEATURE_GPIO | PIN_FEATURE_PWN2, {'A', '1', '2', 0 }, 12, 12},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_13, PIN_FEATURE_GPIO | PIN_FEATURE_PWN3, {'A', '1', '3', 0 }, 13, 13},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_14, PIN_FEATURE_GPIO | PIN_FEATURE_PWN4, {'A', '1', '4', 0 }, 14, 14},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_15, PIN_FEATURE_GPIO | PIN_FEATURE_PWNBREAK, {'A', '1', '5', 0 }, 15, 15},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_16, PIN_FEATURE_NONE, {'A', '1', '6', 0 }, 16, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_17, PIN_FEATURE_NONE, {'A', '1', '7', 0 }, 17, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_18, PIN_FEATURE_NONE, {'A', '1', '8', 0 }, 18, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_19, PIN_FEATURE_NONE, {'A', '1', '9', 0 }, 19, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_20, PIN_FEATURE_NONE, {'A', '2', '0', 0 }, 20, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_21, PIN_FEATURE_NONE, {'A', '2', '1', 0 }, 21, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_22, PIN_FEATURE_NONE, {'A', '2', '2', 0 }, 22, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_23, PIN_FEATURE_NONE, {'A', '2', '3', 0 }, 23, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_24, PIN_FEATURE_NONE, {'A', '2', '4', 0 }, 24, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_25, PIN_FEATURE_NONE, {'A', '2', '5', 0 }, 25, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_26, PIN_FEATURE_NONE, {'A', '2', '6', 0 }, 26, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_27, PIN_FEATURE_NONE, {'A', '2', '7', 0 }, 27, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_28, PIN_FEATURE_NONE, {'A', '2', '8', 0 }, 28, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_29, PIN_FEATURE_NONE, {'A', '2', '9', 0 }, 29, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_30, PIN_FEATURE_NONE, {'A', '3', '0', 0 }, 30, -1},
    {{&machine_pin_type}, GPIOA, GPIO_PIN_31, PIN_FEATURE_NONE, {'A', '3', '1', 0 }, 31, -1},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_0, PIN_FEATURE_GPIO | PIN_FEATURE_PWN0 | PIN_FEATURE_SPI_MISO, {'B', '0', 0, 0 }, 32, 16},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_1, PIN_FEATURE_GPIO | PIN_FEATURE_PWN1 | PIN_FEATURE_SPI_CLK, {'B', '1', 0, 0 }, 33, 17},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_2, PIN_FEATURE_GPIO | PIN_FEATURE_PWN2 | PIN_FEATURE_SPI_CLK, {'B', '2', 0, 0 }, 34, 18},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_3, PIN_FEATURE_GPIO | PIN_FEATURE_PWN3 | PIN_FEATURE_SPI_MISO, {'B', '3', 0, 0 }, 35, 19},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_4, PIN_FEATURE_GPIO | PIN_FEATURE_SPI_CS, {'B', '4', 0, 0 }, 36, 20},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_5, PIN_FEATURE_GPIO | PIN_FEATURE_SPI_MOSI, {'B', '5', 0, 0 }, 37, 21},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_6, PIN_FEATURE_GPIO, {'B', '6', 0, 0 }, 38, 22},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_7, PIN_FEATURE_GPIO, {'B', '7', 0, 0 }, 39, 23},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_8, PIN_FEATURE_GPIO, {'B', '8', 0, 0 }, 40, 24},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_9, PIN_FEATURE_GPIO, {'B', '9', 0, 0 }, 41, 25},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_10, PIN_FEATURE_GPIO, {'B', '1', '0', 0 }, 42, 26},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_11, PIN_FEATURE_GPIO, {'B', '1', '1', 0 }, 43, 27},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_12, PIN_FEATURE_GPIO | PIN_FEATURE_PWN0, {'B', '1', '2', 0 }, 44, 28},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_13, PIN_FEATURE_GPIO | PIN_FEATURE_PWN1, {'B', '1', '3', 0 }, 45, 29},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_14, PIN_FEATURE_GPIO | PIN_FEATURE_PWN2 | PIN_FEATURE_SPI_CS, {'B', '1', '4', 0 }, 46, 30},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_15, PIN_FEATURE_GPIO | PIN_FEATURE_PWN3 | PIN_FEATURE_SPI_CLK, {'B', '1', '5', 0 }, 47, 31},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_16, PIN_FEATURE_GPIO | PIN_FEATURE_PWN4 | PIN_FEATURE_SPI_MISO, {'B', '1', '6', 0 }, 48, 16},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_17, PIN_FEATURE_GPIO | PIN_FEATURE_PWNBREAK | PIN_FEATURE_SPI_MOSI, {'B', '1', '7', 0 }, 49, 17},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_18, PIN_FEATURE_GPIO, {'B', '1', '8', 0 }, 50, 18},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_19, PIN_FEATURE_GPIO | PIN_FEATURE_PWN0, {'B', '1', '9', 0 }, 51, 19},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_20, PIN_FEATURE_GPIO | PIN_FEATURE_PWN1, {'B', '2', '0', 0 }, 52, 20},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_21, PIN_FEATURE_GPIO, {'B', '2', '1', 0 }, 53, 21},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_22, PIN_FEATURE_GPIO, {'B', '2', '2', 0 }, 54, 22},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_23, PIN_FEATURE_GPIO | PIN_FEATURE_SPI_CS, {'B', '2', '3', 0 }, 55, 23},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_24, PIN_FEATURE_GPIO | PIN_FEATURE_PWN2 | PIN_FEATURE_SPI_CLK, {'B', '2', '4', 0 }, 56, 24},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_25, PIN_FEATURE_GPIO | PIN_FEATURE_PWN3 | PIN_FEATURE_SPI_MISO, {'B', '2', '5', 0 }, 57, 25},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_26, PIN_FEATURE_GPIO | PIN_FEATURE_SPI_MOSI, {'B', '2', '6', 0 }, 58, 26},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_27, PIN_FEATURE_GPIO, {'B', '2', '7', 0 }, 59, 27},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_28, PIN_FEATURE_NONE, {'B', '2', '8', 0 }, 60, 28},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_29, PIN_FEATURE_NONE, {'B', '2', '9', 0 }, 61, 29},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_30, PIN_FEATURE_NONE, {'B', '3', '0', 0 }, 62, 30},
    {{&machine_pin_type}, GPIOB, GPIO_PIN_31, PIN_FEATURE_NONE, {'B', '3', '1', 0 }, 63, 31},
};


STATIC inline void pins_toggle_gpio_irq(machine_pin_obj_t * pin_obj, bool enable) {
    if (enable) {
        TDEBUG("pins_toggle_gpio_irq: on");
        __HAL_RCC_GPIO_CLK_ENABLE();
        if (pin_obj->gpio == GPIOA) {
            HAL_NVIC_SetPriority(GPIOA_IRQn, 0);
            HAL_NVIC_EnableIRQ(GPIOA_IRQn);
        } else if (pin_obj->gpio == GPIOB) {
            HAL_NVIC_SetPriority(GPIOB_IRQn, 0);
            HAL_NVIC_EnableIRQ(GPIOB_IRQn);
        }
    } else {
        if (pin_obj->gpio == GPIOA) {
            HAL_NVIC_DisableIRQ(GPIOA_IRQn);
        } else if (pin_obj->gpio == GPIOB) {
            HAL_NVIC_DisableIRQ(GPIOB_IRQn);
        }
        TDEBUG("pins_toggle_gpio_irq: off");
    }
}


void machine_pins_init(void) {
    TDEBUG("machine_pins_init");
    memset(&MP_STATE_VM(machine_pin_irq_handler), 0, sizeof(MP_STATE_VM(machine_pin_irq_handler)));
}

void machine_pins_deinit(void) {
    HAL_NVIC_DisableIRQ(GPIOA_IRQn);
    HAL_NVIC_DisableIRQ(GPIOB_IRQn);
    __HAL_RCC_GPIO_CLK_DISABLE();
    TDEBUG("machine_pins_deinit");
}


STATIC uint32_t machine_pin_get_id(mp_obj_t pin_in) {
    const mp_obj_type_t * obj_type = mp_obj_get_type(pin_in);
    if (obj_type != &machine_pin_type) {
        if (obj_type == &mp_type_str) {
            uint32_t pin_id = pin_str_to_pin_id(mp_obj_str_get_str(pin_in));
            if (pin_id == INVALID_PIN_ID) {
                mp_raise_ValueError(MP_ERROR_TEXT("pin id is invalid"));
            }
            return pin_id;
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
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char prefix = (self->id / 32) + 'A';
    char pinnum = (self->id % 32);
    mp_printf(print, "Pin(%u or '%c%d')", self->id, prefix, pinnum);
}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_mode].u_obj == mp_const_none) {
        mp_raise_ValueError(MP_ERROR_TEXT("PIN 'mode' not set."));
        return mp_const_none;
    }

    // reset the pin to digital if this is a mode-setting init (grab it back from ADC)
    HAL_GPIO_DeInit(self->gpio, self->pin);

    GPIO_InitTypeDef initDef;

    initDef.Pin = self->pin;
    initDef.Mode = mp_obj_get_int(args[ARG_mode].u_obj);
    if (args[ARG_pull].u_obj == mp_const_none) {
        initDef.Pull = GPIO_NOPULL;
    } else {
        initDef.Pull = mp_obj_get_int(args[ARG_pull].u_obj);
    }

    HAL_GPIO_Init(self->gpio, &initDef);

    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
         if (initDef.Mode == GPIO_MODE_OUTPUT) {
            HAL_GPIO_WritePin(self->gpio, self->pin, 
                mp_obj_is_true(args[ARG_value].u_obj) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
    }

    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    uint32_t pin_id = machine_pin_get_id(args[0]);
    if (pin_id == INVALID_PIN_ID) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin name or id"));
    }

    const machine_pin_obj_t *self = NULL;
    for (int i = 0; i < MP_ARRAY_SIZE(machine_pin_obj); i++) {
        if (machine_pin_obj[i].id == pin_id) {
            self = (machine_pin_obj_t *)&machine_pin_obj[i];
            break;
        }
    }

    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin id"));
    }

    if ((self->features & PIN_FEATURE_GPIO) == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }

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
        GPIO_PinState state = HAL_GPIO_ReadPin(self->gpio, self->pin);
        return MP_OBJ_NEW_SMALL_INT(state == GPIO_PIN_SET ? 1:0);
    } else {
        // set pin
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
    enum { ARG_handler, ARG_trigger, ARG_pull, ARG_hard, ARG_enable };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = GPIO_MODE_IT_RISING_FALLING} },
        { MP_QSTR_pull, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_hard, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_bool = false} },
        { MP_QSTR_enable, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_bool = true} },
    };
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get the IRQ object.
    machine_pin_irq_obj_t *irq = MP_STATE_VM(machine_pin_irq_handler[self->irq_slot]);

    // Allocate the IRQ object if it doesn't already exist.
    if (irq == NULL) {
        irq = m_new_obj(machine_pin_irq_obj_t);
        irq->base.base.type = &mp_irq_type;
        irq->base.methods = (mp_irq_methods_t *)&machine_pin_irq_methods;
        irq->base.parent = MP_OBJ_FROM_PTR(self);
        irq->base.handler = mp_const_none;
        irq->base.ishard = false;
        MP_STATE_VM(machine_pin_irq_handler[self->irq_slot]) = irq;
    }

    if (n_args > 1 || kw_args->used != 0) {
        // Configure IRQ.
        
        // Disable all IRQs while data is updated.
        pins_toggle_gpio_irq(self, false);

        // Update IRQ data.
        irq->base.handler = args[ARG_handler].u_obj;
        irq->base.ishard = args[ARG_hard].u_bool;
        irq->trigger = args[ARG_trigger].u_int;
        irq->enabled = args[ARG_enable].u_bool;;
        irq->flags = 0;

        GPIO_InitTypeDef initDef;
        initDef.Pin = self->pin;
        initDef.Mode = irq->trigger;

        int pull = args[ARG_pull].u_int;
        if (pull == -1) {
            if (HAL_IS_BIT_SET(self->gpio->PULLUP_EN, self->pin)) {
                if (HAL_IS_BIT_CLR(self->gpio->PULLDOWN_EN, self->pin)) {
                    pull = GPIO_NOPULL;
                } else {
                    pull = GPIO_PULLDOWN;
                }
            } else {
                if (HAL_IS_BIT_CLR(self->gpio->PULLDOWN_EN, self->pin)) {
                    pull = GPIO_PULLUP;
                } else {
                    TWARN("machine_pin_irq: unhandled pull mode.");
                    pull = GPIO_NOPULL;
                }
            }
        }
        initDef.Pull = pull;
        HAL_GPIO_DeInit(self->gpio, initDef.Pin);
        TDEBUG("machine_pin_irq: Configure IRQ. mode=0x%X, pull=0x%X", initDef.Mode, initDef.Pull);
        HAL_GPIO_Init(self->gpio, &initDef);
        
        // Enable IRQ if a handler is given.
        pins_toggle_gpio_irq(self, true);
    }
    return MP_OBJ_FROM_PTR(irq);
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
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(GPIO_MODE_IT_RISING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(GPIO_MODE_IT_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING_FALLING), MP_ROM_INT(GPIO_MODE_IT_RISING_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_HIGH_LEVEL), MP_ROM_INT(GPIO_MODE_IT_HIGH_LEVEL) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_LOW_LEVEL), MP_ROM_INT(GPIO_MODE_IT_LOW_LEVEL) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);

    switch (request) {
        case MP_PIN_READ: {
            return HAL_GPIO_ReadPin(self->gpio, self->pin) == GPIO_PIN_SET ? 1:0;
        }
        case MP_PIN_WRITE: {
            HAL_GPIO_WritePin(self->gpio, self->pin, arg ? GPIO_PIN_SET : GPIO_PIN_RESET);
            return 0;
        }
    }
    return -1;
}

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

STATIC mp_uint_t machine_pin_irq_trigger(mp_obj_t self_in, mp_uint_t new_trigger) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_pin_irq_obj_t *irq = MP_STATE_VM(machine_pin_irq_handler[self->irq_slot]);
    if (irq) {
        if ((new_trigger == GPIO_MODE_IT_RISING)
            || (new_trigger == GPIO_MODE_IT_FALLING)
            || (new_trigger == GPIO_MODE_IT_RISING_FALLING)
            || (new_trigger == GPIO_MODE_IT_HIGH_LEVEL)
            || (new_trigger == GPIO_MODE_IT_LOW_LEVEL)) {

            pins_toggle_gpio_irq(self, false);

            irq->trigger = new_trigger;
            GPIO_InitTypeDef initDef;
            initDef.Pin = self->pin;
            initDef.Mode = irq->trigger;
            if (HAL_IS_BIT_SET(self->gpio->PULLUP_EN, self->pin)) {
                if (HAL_IS_BIT_CLR(self->gpio->PULLDOWN_EN, self->pin)) {
                    initDef.Pull = GPIO_NOPULL;
                } else {
                    initDef.Pull = GPIO_PULLDOWN;
                }
            } else {
                if (HAL_IS_BIT_CLR(self->gpio->PULLDOWN_EN, self->pin)) {
                    initDef.Pull = GPIO_PULLUP;
                } else {
                    TWARN("machine_pin_irq: unhandled pull mode.");
                    initDef.Pull = GPIO_NOPULL;
                }
            }
            HAL_GPIO_Init(self->gpio, &initDef);

            pins_toggle_gpio_irq(self, true);
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid trigger setting"));
        }
    }
    return 0;
}

STATIC mp_uint_t machine_pin_irq_info(mp_obj_t self_in, mp_uint_t info_type) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_pin_irq_obj_t *irq = MP_STATE_VM(machine_pin_irq_handler[self->irq_slot]);
    if (irq) {
        if (info_type == MP_IRQ_INFO_FLAGS) {
            return irq->flags;
        } else if (info_type == MP_IRQ_INFO_TRIGGERS) {
            return irq->trigger;
        }
    }
    return 0;
}

STATIC mp_uint_t machine_pin_irq_disable(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_pin_irq_obj_t *irq = MP_STATE_VM(machine_pin_irq_handler[self->irq_slot]);
    if (irq) {
        irq->enabled = false;
        return irq->enabled;
    }
    return 0;
}

STATIC mp_uint_t machine_pin_irq_enable(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_pin_irq_obj_t *irq = MP_STATE_VM(machine_pin_irq_handler[self->irq_slot]);
    if (irq) {
        irq->enabled = true;
        return irq->enabled;
    }
    return 0;
}

STATIC const machine_pin_irq_methods_t machine_pin_irq_methods = {
    {
        .trigger = machine_pin_irq_trigger,
        .info = machine_pin_irq_info,
    },
    .disable = machine_pin_irq_disable,
    .enable = machine_pin_irq_enable
};


//--------------------------------------
// implement functions declared in 'mphalport.h'.
const machine_pin_obj_t* mp_hal_get_pin_obj(mp_obj_t pin_in) {
    const mp_obj_type_t * obj_type = mp_obj_get_type(pin_in);
    if (obj_type == &machine_pin_type) {
        return MP_OBJ_TO_PTR(pin_in);
    } else {
        // TODO: 
        uint32_t pin_id = machine_pin_get_id(pin_in);
        if (INVALID_PIN_ID != pin_id) {
            for (int i = 0; i < MP_ARRAY_SIZE(machine_pin_obj); i++) {
                if (machine_pin_obj[i].id == pin_id) {
                    return &machine_pin_obj[i];
                }
            }
        }
    }
    return NULL;
}

void mp_hal_pin_config(mp_hal_pin_obj_t pin_obj, uint32_t mode, uint32_t pull, uint32_t alt) {
    assert(alt == 0);
    GPIO_InitTypeDef initDef;
    initDef.Pin = pin_obj->pin;
    initDef.Mode = mode;
    initDef.Pull = pull;
    HAL_GPIO_Init(pin_obj->gpio, &initDef);
    mp_hal_gpio_clock_enable(pin_obj->gpio);
}


//--------------------------------------
// GPIO IRQ callback
void HAL_GPIO_EXTI_Callback(GPIO_TypeDef * GPIOx, uint32_t pin_id) {
    TDEBUG("HAL_GPIO_EXTI_Callback irq idx: %d", pin_id);
    const machine_pin_obj_t * self = &machine_pin_obj[pin_id];
    if (self->irq_slot != (uint8_t)(-1)) {
        machine_pin_irq_obj_t *irq = MP_STATE_VM(machine_pin_irq_handler[self->irq_slot]);
        if (irq != NULL && irq->enabled) {
            TDEBUG("HAL_GPIO_EXTI_Callback call irq handler!");
            irq->flags = HAL_IS_BIT_SET(GPIOx->DATA, self->pin);
            mp_irq_handler(&irq->base);
        }
    }
}