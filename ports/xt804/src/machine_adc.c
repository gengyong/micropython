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
#include "py/mphal.h"
#include "machine_pin.h"

#include "mphalport.h"

#include <wm_adc.h>


#define INVALID_CHANNEL ((uint32_t)(-1))
#define X804_ADC_CHANNEL_CORE_TEMPERATURE (100)
#define X804_ADC_CHANNEL_CORE_OFFSET (101)

/******************************************************************************/
// MicroPython bindings for machine.ADC

const mp_obj_type_t machine_adc_type;

typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    ADC_HandleTypeDef adc;
} machine_adc_obj_t;

STATIC void machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<ADC channel=%u, freq=%u>", self->adc.Init.channel, self->adc.Init.freq);
}

// ADC(id)
STATIC mp_obj_t machine_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // Check number of arguments
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    mp_obj_t source = all_args[0];

    uint32_t channel = INVALID_CHANNEL;
    if (mp_obj_is_int(source)) {
        // Get and validate channel number.
        mp_int_t num = mp_obj_get_int(source);
        // // if (!((channel >= 0 && channel <= ADC_CHANNEL_TEMPSENSOR) || ADC_IS_VALID_GPIO(channel))) {
        // //     mp_raise_ValueError(MP_ERROR_TEXT("invalid channel"));
        // // }
        switch (num) {
        case 0: channel = ADC_CHANNEL_0; break;
        case 1: channel = ADC_CHANNEL_1; break;
        case 2: channel = ADC_CHANNEL_2; break;
        case 3: channel = ADC_CHANNEL_3; break;
        case 8: channel = ADC_CHANNEL_0_1; break;
        case 9: channel = ADC_CHANNEL_2_3; break;
        case X804_ADC_CHANNEL_CORE_TEMPERATURE: channel = ADC_CHANNEL_TEMP; break;
        case X804_ADC_CHANNEL_CORE_OFFSET: channel = ADC_CHANNEL_OFFSET; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid channel"));
            break;
        }
        if (channel != INVALID_CHANNEL) {
            if (channel == ADC_CHANNEL_0) {
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_1);
            } else if (channel == ADC_CHANNEL_1) {
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_4);
            } else if (channel == ADC_CHANNEL_2) {
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_3);
            } else if (channel == ADC_CHANNEL_3) {
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_2);
            } else if (channel == ADC_CHANNEL_0_1) {
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_1);
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_4);
            } else if (channel == ADC_CHANNEL_2_3) {
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_3);
                __HAL_AFIO_REMAP_ADC(GPIOA, GPIO_PIN_2);
            }
        }
    } else {
        // Get GPIO and check it has ADC capabilities.
        mp_hal_pin_obj_t pin_obj = mp_hal_get_pin_obj(source);
        if (!pin_obj) {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid Pin"));
        } else {
            if (pin_obj->features & PIN_FEATURE_MASK_ADC) {
                if (pin_obj->features & PIN_FEATURE_ADC0) {
                    channel = ADC_CHANNEL_0;
                } else if (pin_obj->features & PIN_FEATURE_ADC1) {
                    channel = ADC_CHANNEL_1;
                } else if (pin_obj->features & PIN_FEATURE_ADC2) {
                    channel = ADC_CHANNEL_2;
                } else if (pin_obj->features & PIN_FEATURE_ADC3) {
                    channel = ADC_CHANNEL_3;
                } else {
                    assert("Unknow ADC channel on specify Pin.");
                    mp_raise_ValueError(MP_ERROR_TEXT("Unknow ADC channel on specify Pin."));
                }
                if (channel != INVALID_CHANNEL) {
                    __HAL_AFIO_REMAP_ADC(pin_obj->gpio, pin_obj->pin);
                }
            } else {
                mp_raise_ValueError(MP_ERROR_TEXT("Pin doesn't have ADC capabilities"));
            }
        }
    }

    TDEBUG("select channel:%u", channel);

    

    // // Initialise the ADC peripheral if it's not already running.
    // // if (!(adc_hw->cs & ADC_CS_EN_BITS)) {
    // //     adc_init();
    // // }
    // // if (ADC_IS_VALID_GPIO(channel)) {
    // //     // Configure the GPIO pin in ADC mode.
    // //     adc_gpio_init(channel);
    // //     channel = ADC_CHANNEL_FROM_GPIO(channel);
    // // } else if (channel == ADC_CHANNEL_TEMPSENSOR) {
    // //     // Enable temperature sensor.
    // //     adc_set_temp_sensor_enabled(1);
    // // }

    // Create ADC object.
    if (channel == INVALID_CHANNEL) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid ADC channel."));
    }

    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIO_CLK_ENABLE();

    machine_adc_obj_t *o = m_new_obj(machine_adc_obj_t);
    o->base.type = &machine_adc_type;
    o->adc.Instance = ADC;
    o->adc.Init.channel = channel;
    o->adc.Init.freq = 1000; // TODO:
    //o->adc.Init.cmp_val;
    //o->adc.Init.cmp_pol;
    TDEBUG("HAL_ADC_Init...");
    if (HAL_ADC_Init(&(o->adc)) != HAL_OK) {
        TDEBUG("HAL_ADC_Init failed");
        mp_raise_ValueError(MP_ERROR_TEXT("HAL init ADC failed."));
    }
    TDEBUG("HAL_ADC_Init success");

    return MP_OBJ_FROM_PTR(o);
}

// read_u16()
STATIC mp_obj_t machine_adc_read_u16(mp_obj_t self_in) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);

    HAL_ADC_Start(&(self->adc));
    HAL_ADC_PollForConversion(&(self->adc));
    HAL_ADC_Stop(&(self->adc));
    int value = HAL_ADC_GetValue(&(self->adc));

    //uint16_t final = (value + 4784000000/126363) * 65536 / (9600000000/126363);
    TDEBUG("ADC:%d", value);
    int final = (value * 0.86263808) + 32658.773333333;
    final = MAX(0, MIN(final, 0xFFFF));
    return MP_OBJ_NEW_SMALL_INT(final);
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_read_u16_obj, machine_adc_read_u16);

// read_voltage
STATIC mp_obj_t machine_adc_read_voltage(mp_obj_t self_in) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int value = HAL_ADC_GET_INPUT_VOLTAGE(&(self->adc));
    if (self->adc.Init.channel == ADC_CHANNEL_0_1 || self->adc.Init.channel == ADC_CHANNEL_2_3) {
        value -= 1200; // Differential voltage, range[-1.2, 1.2]
    }
    return MP_OBJ_NEW_SMALL_INT(value);
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_read_voltage_obj, machine_adc_read_voltage);

STATIC const mp_rom_map_elem_t machine_adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_u16), MP_ROM_PTR(&machine_adc_read_u16_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_voltage), MP_ROM_PTR(&machine_adc_read_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_CORE_TEMP), MP_ROM_INT(X804_ADC_CHANNEL_CORE_TEMPERATURE) },
    { MP_ROM_QSTR(MP_QSTR_CORE_OFFSET), MP_ROM_INT(X804_ADC_CHANNEL_CORE_OFFSET) },
};
STATIC MP_DEFINE_CONST_DICT(machine_adc_locals_dict, machine_adc_locals_dict_table);

const mp_obj_type_t machine_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .print = machine_adc_print,
    .make_new = machine_adc_make_new,
    .locals_dict = (mp_obj_dict_t *)&machine_adc_locals_dict,
};
