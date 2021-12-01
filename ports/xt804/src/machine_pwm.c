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

#include <wm_pwm.h>

#include "modmachine.h"
#include "machine_pin.h"
#include "extmod/machine_pwm.h"

/******************************************************************************/
// MicroPython bindings for machine.PWM

typedef void(*afio_mapping_func_t)(GPIO_TypeDef *, uint32_t);

typedef struct _machine_pwm_obj_t {
    mp_obj_base_t base;
    afio_mapping_func_t mapping;
    uint8_t channel;
    uint32_t feature;
} machine_pwm_obj_t;

static void afio_mapping_pwn0_to_pin(GPIO_TypeDef * gpio, uint32_t pin) {
    __HAL_AFIO_REMAP_PWM0(gpio, pin);
}

static void afio_mapping_pwn1_to_pin(GPIO_TypeDef * gpio, uint32_t pin) {
    __HAL_AFIO_REMAP_PWM1(gpio, pin);
}

static void afio_mapping_pwn2_to_pin(GPIO_TypeDef * gpio, uint32_t pin) {
    __HAL_AFIO_REMAP_PWM2(gpio, pin);
}

static void afio_mapping_pwn3_to_pin(GPIO_TypeDef * gpio, uint32_t pin) {
    __HAL_AFIO_REMAP_PWM3(gpio, pin);
}

static void afio_mapping_pwn4_to_pin(GPIO_TypeDef * gpio, uint32_t pin) {
    __HAL_AFIO_REMAP_PWM4(gpio, pin);
}

STATIC machine_pwm_obj_t machine_pwm_obj[] = {
    {{&machine_pwm_type}, afio_mapping_pwn0_to_pin, PWM_CHANNEL_0, PIN_FEATURE_PWN0 },
    {{&machine_pwm_type}, afio_mapping_pwn1_to_pin, PWM_CHANNEL_1, PIN_FEATURE_PWN1 },
    {{&machine_pwm_type}, afio_mapping_pwn2_to_pin, PWM_CHANNEL_2, PIN_FEATURE_PWN2 },
    {{&machine_pwm_type}, afio_mapping_pwn3_to_pin, PWM_CHANNEL_3, PIN_FEATURE_PWN3 },
    {{&machine_pwm_type}, afio_mapping_pwn4_to_pin, PWM_CHANNEL_4, PIN_FEATURE_PWN4 },
};

void mp_machine_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //mp_printf(print, "<PWM slice=%u channel=%u>", self->slice, self->channel);
}

// PWM(pin)
mp_obj_t mp_machine_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // Check number of arguments
    mp_arg_check_num(n_args, n_kw, 2, 3, false);

    enum { ARG_channel, ARG_mode };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel, MP_ARG_INT, {.u_int = PWM_CHANNEL_0} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_int = PWM_OUT_MODE_INDEPENDENT} },
    };
    const machine_pin_obj_t * pin = MP_OBJ_TO_PTR(all_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, all_args + 1, NULL, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get PWM channel and mode.
    mp_int_t channel = args[ARG_channel].u_int;
    mp_int_t mode = args[ARG_mode].u_int;

    if (mode == PWM_OUT_MODE_2COMPLEMENTARY || mode == PWM_OUT_MODE_2SYNC) {
        if (channel != 0 && channel != 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("specify mode '2COMPLEMENTARY' or '2SYNC' only works on PWM0 and PWM2."));
        }
    }

    // Get static peripheral object.
    if (channel < 0 || channel >= sizeof(machine_pwm_obj)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid PWM channel[PWM0 - PWM4]."));
    }

    const machine_pwm_obj_t *self = &machine_pwm_obj[channel];
    if ((self->feature & pin->features) == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("this Pin doesn't support specify PWM channel."));
    }

    self->mapping(pin->gpio, pin->pin);

    // Select PWM function for given GPIO.
    //gpio_set_function(gpio, GPIO_FUNC_PWM);

    PWM_HandleTypeDef pwm;
    pwm.Instance = PWM;
    pwm.Init.AutoReloadPreload = PWM_AUTORELOAD_PRELOAD_ENABLE;
    pwm.Init.CounterMode = PWM_COUNTERMODE_EDGEALIGNED_DOWN;
    pwm.Init.Prescaler = 4;
    pwm.Init.Period = 99;    // Frequency = 40,000,000 / 4 / (99 + 1) = 100,000 = 100KHz
    pwm.Init.Pulse = 19;     // Duty Cycle = (19 + 1) / (99 + 1) = 20%
    pwm.Init.OutMode = PWM_OUT_MODE_2COMPLEMENTARY; // 2-channel complementary mode
    HAL_PWM_Init(&pwm);

    __HAL_RCC_PWM_CLK_ENABLE();

    return MP_OBJ_FROM_PTR(self);
}

void mp_machine_pwm_deinit(machine_pwm_obj_t *self) {
    //pwm_set_enabled(self->slice, false);
    //HAL_PWM_DeInit(self->pwm);
}

mp_obj_t mp_machine_pwm_freq_get(machine_pwm_obj_t *self) {
    // uint32_t source_hz = clock_get_hz(clk_sys);
    // uint32_t div16 = pwm_hw->slice[self->slice].div;
    // uint32_t top = pwm_hw->slice[self->slice].top;
    // uint32_t pwm_freq = 16 * source_hz / div16 / top;
    // return MP_OBJ_NEW_SMALL_INT(pwm_freq);
    return MP_OBJ_NEW_SMALL_INT(1);
}

void mp_machine_pwm_freq_set(machine_pwm_obj_t *self, mp_int_t freq) {
    // Set the frequency, making "top" as large as possible for maximum resolution.
    // Maximum "top" is set at 65534 to be able to achieve 100% duty with 65535.
    // #define TOP_MAX 65534
    // uint32_t source_hz = clock_get_hz(clk_sys);
    // uint32_t div16_top = 16 * source_hz / freq;
    // uint32_t top = 1;
    // for (;;) {
    //     // Try a few small prime factors to get close to the desired frequency.
    //     if (div16_top >= 16 * 5 && div16_top % 5 == 0 && top * 5 <= TOP_MAX) {
    //         div16_top /= 5;
    //         top *= 5;
    //     } else if (div16_top >= 16 * 3 && div16_top % 3 == 0 && top * 3 <= TOP_MAX) {
    //         div16_top /= 3;
    //         top *= 3;
    //     } else if (div16_top >= 16 * 2 && top * 2 <= TOP_MAX) {
    //         div16_top /= 2;
    //         top *= 2;
    //     } else {
    //         break;
    //     }
    // }
    // if (div16_top < 16) {
    //     mp_raise_ValueError(MP_ERROR_TEXT("freq too large"));
    // } else if (div16_top >= 256 * 16) {
    //     mp_raise_ValueError(MP_ERROR_TEXT("freq too small"));
    // }
    // pwm_hw->slice[self->slice].div = div16_top;
    // pwm_hw->slice[self->slice].top = top;

    //HAL_PWM_Freq_Set(self->pwm, )
}

mp_obj_t mp_machine_pwm_duty_get_u16(machine_pwm_obj_t *self) {
    // uint32_t top = pwm_hw->slice[self->slice].top;
    // uint32_t cc = pwm_hw->slice[self->slice].cc;
    // cc = (cc >> (self->channel ? PWM_CH0_CC_B_LSB : PWM_CH0_CC_A_LSB)) & 0xffff;
    //return MP_OBJ_NEW_SMALL_INT(cc * 65535 / (top + 1));
    return MP_OBJ_NEW_SMALL_INT(1);
}

void mp_machine_pwm_duty_set_u16(machine_pwm_obj_t *self, mp_int_t duty_u16) {
    // uint32_t top = pwm_hw->slice[self->slice].top;
    // uint32_t cc = duty_u16 * (top + 1) / 65535;
    // pwm_set_chan_level(self->slice, self->channel, cc);
    // pwm_set_enabled(self->slice, true);
}

mp_obj_t mp_machine_pwm_duty_get_ns(machine_pwm_obj_t *self) {
    // uint32_t source_hz = clock_get_hz(clk_sys);
    // uint32_t slice_hz = 16 * source_hz / pwm_hw->slice[self->slice].div;
    // uint32_t cc = pwm_hw->slice[self->slice].cc;
    // cc = (cc >> (self->channel ? PWM_CH0_CC_B_LSB : PWM_CH0_CC_A_LSB)) & 0xffff;
    // return MP_OBJ_NEW_SMALL_INT((uint64_t)cc * 1000000000ULL / slice_hz);
    return MP_OBJ_NEW_SMALL_INT((uint64_t)1);
}

void mp_machine_pwm_duty_set_ns(machine_pwm_obj_t *self, mp_int_t duty_ns) {
    // uint32_t source_hz = clock_get_hz(clk_sys);
    // uint32_t slice_hz = 16 * source_hz / pwm_hw->slice[self->slice].div;
    // uint32_t cc = (uint64_t)duty_ns * slice_hz / 1000000000ULL;
    // if (cc > 65535) {
    //     mp_raise_ValueError(MP_ERROR_TEXT("duty larger than period"));
    // }
    // pwm_set_chan_level(self->slice, self->channel, cc);
    // pwm_set_enabled(self->slice, true);
}
