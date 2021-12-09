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
#include "py/mperrno.h"


#define XT804_PWM_PERIOD_MAX    (0x0FF)


static inline bool PWM_Freq_Get(uint32_t channel, uint32_t * prescaler, uint32_t * period) {
    assert(prescaler != NULL);
    assert(period != NULL);
    if (channel == PWM_CHANNEL_0) {
        *prescaler = (READ_REG(PWM->CLKDIV01) & PWM_CLKDIV01_CH0);
        *period = (READ_REG(PWM->PERIOD) & PWM_PERIOD_CH0);
        return true;
    } else if (channel == PWM_CHANNEL_1) {
        *prescaler = (READ_REG(PWM->CLKDIV01) & PWM_CLKDIV01_CH1) >> PWM_CLKDIV01_CH1_Pos;
        *period = (READ_REG(PWM->PERIOD) & PWM_PERIOD_CH1) >> PWM_PERIOD_CH1_Pos;
        return true;
    } else if (channel == PWM_CHANNEL_2) {
        *prescaler = (READ_REG(PWM->CLKDIV23) & PWM_CLKDIV01_CH1) >> PWM_CLKDIV23_CH2_Pos;
        *period = (READ_REG(PWM->PERIOD) & PWM_PERIOD_CH2) >> PWM_PERIOD_CH2_Pos;
        return true;
    } else if (channel == PWM_CHANNEL_3) {
        *prescaler = (READ_REG(PWM->CLKDIV23) & PWM_CLKDIV23_CH3) >> PWM_CLKDIV23_CH3_Pos;
        *period = (READ_REG(PWM->PERIOD) & PWM_PERIOD_CH3) >> PWM_PERIOD_CH3_Pos;
        return true;
    } else if (channel == PWM_CHANNEL_4) {
        uint32_t bits = (READ_REG(PWM->CH4CR2) & (PWM_CH4CR1_DIV | PWM_CH4CR1_PRD));
        *prescaler = (bits >> PWM_CH4CR1_DIV_Pos) & 0xFFFF;
        *period = (bits >> PWM_CH4CR1_PRD_Pos) & 0xFF;
        return true;
    }
    return false;
}

static inline bool PWM_Pulse_Get(uint32_t channel, uint32_t * pulse) {
    assert(pulse != NULL);
    if (channel == PWM_CHANNEL_0) {
        *pulse = (READ_REG(PWM->CMPDAT) & PWM_CMPDAT_CH0);
        return true;
    } else if (channel == PWM_CHANNEL_1) {
        *pulse = (READ_REG(PWM->CMPDAT) & PWM_CMPDAT_CH1) >> PWM_CMPDAT_CH1_Pos;
        return true;
    } else if (channel == PWM_CHANNEL_2) {
        *pulse = (READ_REG(PWM->CMPDAT) & PWM_CMPDAT_CH2) >> PWM_CMPDAT_CH2_Pos;
        return true;
    } else if (channel == PWM_CHANNEL_3) {
        *pulse = (READ_REG(PWM->CMPDAT) & PWM_CMPDAT_CH3) >> PWM_CMPDAT_CH3_Pos;
        return true;
    } else if (channel == PWM_CHANNEL_4) {
        *pulse = (READ_REG(PWM->CH4CR2) & PWM_CH4CR2_CMP) >> PWM_CH4CR2_CMP_Pos;
        return true;
    }
    return false;
}

static inline bool PWM_Calc_Prescaler_Period_By_Freq(uint32_t freq, uint32_t * prescaler, uint32_t * period) {
    assert(prescaler != NULL);
    assert(period != NULL);
    if (freq > 0) {
        uint32_t top = APB_CLK / freq;
        if (top > XT804_PWM_PERIOD_MAX) {
            uint32_t div = (top + XT804_PWM_PERIOD_MAX) / XT804_PWM_PERIOD_MAX;
            *period = (top / div) - 1;
            *prescaler = div;
        } else {
            *period = top > 1 ? top - 1: 1;
            *prescaler = 1;
        }
        return true;
    }
    return false;
}


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

const mp_obj_type_t machine_pwm_type;

STATIC const machine_pwm_obj_t machine_pwm_obj[] = {
    {{&machine_pwm_type}, afio_mapping_pwn0_to_pin, PWM_CHANNEL_0, PIN_FEATURE_PWN0 },
    {{&machine_pwm_type}, afio_mapping_pwn1_to_pin, PWM_CHANNEL_1, PIN_FEATURE_PWN1 },
    {{&machine_pwm_type}, afio_mapping_pwn2_to_pin, PWM_CHANNEL_2, PIN_FEATURE_PWN2 },
    {{&machine_pwm_type}, afio_mapping_pwn3_to_pin, PWM_CHANNEL_3, PIN_FEATURE_PWN3 },
    {{&machine_pwm_type}, afio_mapping_pwn4_to_pin, PWM_CHANNEL_4, PIN_FEATURE_PWN4 },
};

void mp_machine_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<PWM channel=%u>", self->channel);
}

// PWM(pin)
mp_obj_t mp_machine_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // Check number of arguments
    mp_arg_check_num(n_args, n_kw, 1, 10, true);

    enum { ARG_mode, ARG_freq, ARG_auto_reload_preload, ARG_counter_mode, ARG_prescaler, ARG_period, ARG_pulse, ARG_dtdiv, ARG_dtcnt };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = PWM_OUT_MODE_INDEPENDENT} },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_auto_reload_preload, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_counter_mode, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = PWM_COUNTERMODE_EDGEALIGNED_DOWN} },
        { MP_QSTR_prescaler, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_period, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_pulse, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_dtdiv, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_dtcnt, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} }
    };

    const machine_pin_obj_t * pin = MP_OBJ_TO_PTR(all_args[0]);
   
    // Get PWM channel.
    mp_int_t pwnchan = 0;
    if (pin->features & PIN_FEATURE_PWN0) {
        pwnchan = 0;
    } else if (pin->features & PIN_FEATURE_PWN1) {
        pwnchan = 1;
    } else if (pin->features & PIN_FEATURE_PWN2) {
        pwnchan = 2;
    } else if (pin->features & PIN_FEATURE_PWN3) {
        pwnchan = 3;
    } else if (pin->features & PIN_FEATURE_PWN4) {
        pwnchan = 4;
    } else {
        // TODO: use software PWM.
        mp_raise_ValueError(MP_ERROR_TEXT("specify Pin don't support hardward PWM."));
    }


    TDEBUG("0000000000000000000000000 n_args:%u", n_args);

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args - 1, n_kw, all_args + 1, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    TDEBUG("1111111111111111111111111111");
    TDEBUG("channel:%d, mode:%d, freq: %d, auto_reload_preload:%d, counter_mode:%d, prescaler:%d, period:%d, pulse:%d, dtdiv:%d, dtcnt:%d",
        pwnchan,
        args[ARG_mode].u_int,
        args[ARG_freq].u_obj == mp_const_none ? -1 : mp_obj_get_int(args[ARG_freq].u_obj),
        args[ARG_auto_reload_preload].u_bool,
        args[ARG_counter_mode].u_int,
        args[ARG_prescaler].u_obj == mp_const_none ? -1 :mp_obj_get_int(args[ARG_prescaler].u_obj),
        args[ARG_period].u_obj == mp_const_none ? -1 :mp_obj_get_int(args[ARG_period].u_obj),
        args[ARG_pulse].u_obj == mp_const_none ? -1 :mp_obj_get_int(args[ARG_pulse].u_obj),
        args[ARG_dtdiv].u_obj == mp_const_none ? -1 :mp_obj_get_int(args[ARG_dtdiv].u_obj),
        args[ARG_dtcnt].u_obj == mp_const_none ? -1 :mp_obj_get_int(args[ARG_dtcnt].u_obj)
    );
   

    mp_int_t mode = args[ARG_mode].u_int;
    if (!IS_PWM_OUTMODE(mode)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid PWM out mode."));
        
    }

    TDEBUG("333333333333333333333");

    if (mode == PWM_OUT_MODE_2COMPLEMENTARY || mode == PWM_OUT_MODE_2SYNC) {
        if (pwnchan != 0 && pwnchan != 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("specify mode '2COMPLEMENTARY' or '2SYNC' only works on PWM0 and PWM2."));
        }
    } else if (mode == PWM_OUT_MODE_5SYNC) {
        if (pwnchan != 0) {
            mp_warning(MP_WARN_CAT(RuntimeWarning), "PWM Out Mode 5SYNC must be bind to PWM Channel0, auto switched to Channel0... Done.");
        }
        pwnchan = 0;
    }

    TDEBUG("4444444444444444444444");

    const machine_pwm_obj_t * self = &machine_pwm_obj[pwnchan];
    if ((self->feature & pin->features) == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("this Channel doesn't support specify PWM out mode."));
    }

    TDEBUG("5555555555555555555");
    
    mp_int_t counter_mode = args[ARG_counter_mode].u_int;
    if (!IS_PWM_COUNTER_MODE(counter_mode)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid PWM counter mode."));
    }

    TDEBUG("66666666666666666");


    bool prescaler_got = false;
    bool period_got = false;
    mp_int_t prescaler = 0;
    mp_int_t period = 0;
    if (args[ARG_freq].u_obj == mp_const_none) {
        TDEBUG("7777777777777777777");
        // parse prescaler and period
        prescaler_got = mp_obj_get_int_maybe(args[ARG_prescaler].u_obj, &prescaler);
        period_got = mp_obj_get_int_maybe(args[ARG_period].u_obj, &period);

        TDEBUG("8888888888888888888:prescaler:%d period:%d", prescaler, period);
        if (!prescaler_got && !period_got) {
            mp_raise_ValueError(MP_ERROR_TEXT("must create PWM with 'freq' setting or with `prescaler' & `period' setting."));
        } else if (prescaler_got && !period_got) {
            mp_raise_ValueError(MP_ERROR_TEXT("missing param `period`."));
        } else if (!prescaler_got && period_got) {
            mp_raise_ValueError(MP_ERROR_TEXT("missing param `prescaler`."));
        }
        if (!IS_PWM_PRESCALER(prescaler)) {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid `prescaler` value, should be in range [0 ~ 65535]."));
        }
        if (!IS_PWM_PERIOD(period)) {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid `period` value, should be in range [0 ~ 255]."));
        }
    } else {
        TDEBUG("99999999999999999999");
        mp_int_t freq = mp_obj_int_get_checked(args[ARG_freq].u_obj);
        if (freq <= 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid `freq` value, should be in range [1 ~ 400000000]."));
        } else {
            TDEBUG("aaaaaaaaaaaaaaaaaaa");
            uint32_t prescaler_u32 = 0;
            uint32_t period_u32 = 0;
            if (PWM_Calc_Prescaler_Period_By_Freq(freq, &prescaler_u32, &period_u32)) {
                prescaler = prescaler_u32;
                period = period_u32;
                prescaler_got = true;
                period_got = true;
            }
        }
    }

    TDEBUG("PWM set prescaler=%u period=%u", prescaler, period);

    if (prescaler_got && period_got) {
        // init PWM
        mp_int_t pulse = 0;
        if (!mp_obj_get_int_maybe(args[ARG_pulse].u_obj, &pulse)) {
            // default to half period
            pulse = ((period + 1) >> 1) - 1;
        }

        PWM_HandleTypeDef pwm;
        pwm.Instance = PWM;
        pwm.Channel = self->channel;
        pwm.Init.OutMode = mode;
        pwm.Init.CounterMode = counter_mode;
        pwm.Init.AutoReloadPreload = args[ARG_auto_reload_preload].u_bool ? PWM_AUTORELOAD_PRELOAD_ENABLE : PWM_AUTORELOAD_PRELOAD_DISABLE;
        pwm.Init.Prescaler = prescaler;
        pwm.Init.Period = period;
        pwm.Init.Pulse = pulse;

        if (mode == PWM_OUT_MODE_2COMPLEMENTARY) {
            mp_int_t dtdiv = 0;
            mp_int_t dtcnt = 0;
            if (!mp_obj_get_int_maybe(args[ARG_dtdiv].u_obj, &dtdiv)) {
                mp_raise_ValueError(MP_ERROR_TEXT("must set param `dtdiv` for PWM out mode `2COMPLEMENTARY`."));
            }

            switch(dtdiv) {
            case 0: case 1: pwm.Init.Dtdiv = PWM_DTDIV_NONE; break;
            case 2: pwm.Init.Dtdiv = PWM_DTDIV_2; break;
            case 4: pwm.Init.Dtdiv = PWM_DTDIV_4; break;
            case 8: pwm.Init.Dtdiv = PWM_DTDIV_8; break;
            default:
                mp_raise_ValueError(MP_ERROR_TEXT("invalid `dtdiv` value, should be one of [1, 2, 4, 8]."));
            }

            if (!mp_obj_get_int_maybe(args[ARG_dtcnt].u_obj, &dtcnt)) {
                mp_raise_ValueError(MP_ERROR_TEXT("must set param `dtdiv` for PWM out mode `2COMPLEMENTARY`."));
            }
            if (!IS_PWM_DTCNT(dtcnt)) {
                mp_raise_ValueError(MP_ERROR_TEXT("invalid `dtcnt` value, should be in range [0 ~ 255]."));
            }
            
            pwm.Init.Dtcnt = dtcnt;
        }
        
        TDEBUG("PWM mapping and start: pulse:%u", pulse);

        __HAL_RCC_PWM_CLK_ENABLE();
        self->mapping(pin->gpio, pin->pin);
        if (HAL_PWM_Init(&pwm) != HAL_OK) {
            mp_raise_OSError(MP_EIO);
        }
        if (HAL_PWM_Start(&pwm, self->channel) != HAL_OK) {
            mp_raise_OSError(MP_EIO);
        }
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("could not resolve PWN freq, operation failed."));
    }

    return MP_OBJ_FROM_PTR(self);
}

void mp_machine_pwm_deinit(machine_pwm_obj_t *self) {
    TDEBUG("mp_machine_pwm_deinit: channel:%u", self->channel);
    PWM_HandleTypeDef pwm;
    pwm.Instance = PWM;
    pwm.Channel = self->channel;
    HAL_PWM_Stop(&pwm, self->channel);

    if (!READ_BIT(PWM->CR, (PWM_CHANNEL_ALL << PWM_CR_CNTEN_Pos))) {
        // if no PWN works, disable PWM clk.
        TDEBUG("HALT PWM CLK");
        __HAL_RCC_PWM_CLK_DISABLE();
    }
}

mp_obj_t mp_machine_pwm_freq_get(machine_pwm_obj_t *self) {
    uint32_t prescaler = 0;
    uint32_t period = 0;
    if (PWM_Freq_Get(self->channel, &prescaler, &period)) {
        TDEBUG("mp_machine_pwm_freq_get: prescaler:%u, period:%u", prescaler, period);
        uint32_t len = prescaler * (period + 1);
        if (len > 0) {
            return MP_OBJ_NEW_SMALL_INT(APB_CLK / len);
        }
    }
    return MP_OBJ_NEW_SMALL_INT(0);
}

void mp_machine_pwm_freq_set(machine_pwm_obj_t *self, mp_int_t freq) {
    uint32_t prescaler = 0;
    uint32_t period = 0;

    float ratio = 0;
    if (PWM_Freq_Get(self->channel, &prescaler, &period)) {
        uint32_t pulse = 0;
        if (PWM_Pulse_Get(self->channel, &pulse)) {
            ratio = ((pulse + 1.0) / (period + 1.0));
            TDEBUG("mp_machine_pwm_freq_set: ratio:%f", ratio);
        }
    }

    if (PWM_Calc_Prescaler_Period_By_Freq(freq, &prescaler, &period)) {
        uint32_t pulse = period * ratio;
        
        PWM_HandleTypeDef pwm;
        pwm.Instance = PWM;
        pwm.Channel = self->channel;
        pwm.Init.Prescaler = prescaler;
        pwm.Init.Period = period;
        pwm.Init.Pulse = pulse;

        if (HAL_OK != HAL_PWM_Freq_Set(&pwm, self->channel, prescaler, period)) {
            mp_raise_OSError(MP_EIO);
        }
        
        if (HAL_OK != HAL_PWM_Duty_Set(&pwm, self->channel, pulse)) {
            mp_raise_OSError(MP_EIO);
        }

        TDEBUG("mp_machine_pwm_freq_set: freq:%d => prescaler:%u, period:%u, pulse:%u",
            freq, prescaler, period, pulse
        );

    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid `freq' setting."));
    }
}

mp_obj_t mp_machine_pwm_duty_get(machine_pwm_obj_t *self) {
    uint32_t duty = 0;
    if (PWM_Pulse_Get(self->channel, &duty)) {
        return MP_OBJ_NEW_SMALL_INT(duty);
    }
    return MP_OBJ_NEW_SMALL_INT(0);
}

void mp_machine_pwm_duty_set(machine_pwm_obj_t *self, mp_int_t duty) {
    // uint32_t top = pwm_hw->slice[self->slice].top;
    // uint32_t cc = duty_u16 * (top + 1) / 65535;
    // pwm_set_chan_level(self->slice, self->channel, cc);
    // pwm_set_enabled(self->slice, true);
 
    PWM_HandleTypeDef pwm;
    pwm.Instance = PWM;
    pwm.Channel = self->channel;
    if (HAL_OK != HAL_PWM_Duty_Set(&pwm, self->channel, duty)) {
        mp_raise_OSError(MP_EIO);
    }
}

mp_obj_t mp_machine_pwm_duty_get_u16(machine_pwm_obj_t *self) {
    uint32_t prescaler = 0;
    uint32_t period = 0;
    if (PWM_Freq_Get(self->channel, &prescaler, &period)) {
        uint32_t pulse = 0;
         
        if (PWM_Pulse_Get(self->channel, &pulse)) {
            TDEBUG("mp_machine_pwm_duty_get_u16: prescaler:%u, period:%u, pulse:%u",
                prescaler, period, pulse
            );
            return MP_OBJ_NEW_SMALL_INT((pulse + 1) * (UINT16_MAX + 1) / (period + 1));
        }
    }
    return MP_OBJ_NEW_SMALL_INT(0);
}

void mp_machine_pwm_duty_set_u16(machine_pwm_obj_t *self, mp_int_t duty_u16) {
    // uint32_t top = pwm_hw->slice[self->slice].top;
    // uint32_t cc = duty_u16 * (top + 1) / 65535;
    // pwm_set_chan_level(self->slice, self->channel, cc);
    // pwm_set_enabled(self->slice, true);
    uint32_t prescaler = 0;
    uint32_t period = 0;
    if (PWM_Freq_Get(self->channel, &prescaler, &period)) {
        uint32_t pulse = duty_u16 * (period + 1) / (UINT16_MAX + 1);

        TDEBUG("mp_machine_pwm_duty_set_u16: duty_u16:%d, result: prescaler:%u, period:%u, pulse: %u",
            duty_u16, prescaler, period, pulse
        );

        PWM_HandleTypeDef pwm;
        pwm.Instance = PWM;
        pwm.Channel = self->channel;
        if (HAL_OK != HAL_PWM_Duty_Set(&pwm, self->channel, pulse)) {
            mp_raise_OSError(MP_EIO);
        }
    } else {
        mp_raise_OSError(MP_EIO);
    }
}

mp_obj_t mp_machine_pwm_duty_get_ns(machine_pwm_obj_t *self) {
    // uint32_t source_hz = clock_get_hz(clk_sys);
    // uint32_t slice_hz = 16 * source_hz / pwm_hw->slice[self->slice].div;
    // uint32_t cc = pwm_hw->slice[self->slice].cc;
    // cc = (cc >> (self->channel ? PWM_CH0_CC_B_LSB : PWM_CH0_CC_A_LSB)) & 0xffff;
    // return MP_OBJ_NEW_SMALL_INT((uint64_t)cc * 1000000000ULL / slice_hz);

    uint32_t prescaler = 0;
    uint32_t period = 0;
    if (PWM_Freq_Get(self->channel, &prescaler, &period)) {
        uint32_t pulse = 0;
        if (PWM_Pulse_Get(self->channel, &pulse)) {
            return MP_OBJ_NEW_SMALL_INT(((pulse + 1) * prescaler) * 1000000000ULL / APB_CLK);
        }
    }
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
    uint32_t prescaler = 0;
    uint32_t period = 0;
    if (PWM_Freq_Get(self->channel, &prescaler, &period)) {
        uint32_t pulse = (duty_ns * (APB_CLK/1000000000ULL)) / ((period + 1) * prescaler);

        TDEBUG("mp_machine_pwm_duty_set_ns: duty_ns:%d, result: prescaler:%u, period:%u, pulse: %u",
            duty_ns, prescaler, period, pulse
        );

        PWM_HandleTypeDef pwm;
        pwm.Instance = PWM;
        pwm.Channel = self->channel;
        if (HAL_OK != HAL_PWM_Duty_Set(&pwm, self->channel, pulse)) {
            mp_raise_OSError(MP_EIO);
        }
    } else {
        mp_raise_OSError(MP_EIO);
    }
}

// PWM.deinit()
STATIC mp_obj_t machine_pwm_deinit(mp_obj_t self_in) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_machine_pwm_deinit(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pwm_deinit_obj, machine_pwm_deinit);


// PWM.info()
STATIC mp_obj_t machine_pwm_info(mp_obj_t self_in) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t prescaler = 0;
    uint32_t period = 0;
    if (PWM_Freq_Get(self->channel, &prescaler, &period)) {
        uint32_t pulse = 0;
        if (PWM_Pulse_Get(self->channel, &pulse)) {
            TDEBUG("prescaler:%u period:%u pulse:%u", prescaler,  period, pulse);
            mp_obj_t tuple[3] = {
                mp_obj_new_int(prescaler),
                mp_obj_new_int(period),
                mp_obj_new_int(pulse)
            };
            return mp_obj_new_tuple(3, tuple);
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pwm_info_obj, machine_pwm_info);

// PWM.freq([value])
STATIC mp_obj_t machine_pwm_freq(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // Get frequency.
        return mp_machine_pwm_freq_get(self);
    } else {
        // Set the frequency.
        mp_int_t freq = mp_obj_get_int(args[1]);
        mp_machine_pwm_freq_set(self, freq);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_freq_obj, 1, 2, machine_pwm_freq);

// PWM.duty([duty])
STATIC mp_obj_t machine_pwm_duty(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // Get duty cycle.
        return mp_machine_pwm_duty_get(self);
    } else {
        // Set duty cycle.
        mp_int_t duty = mp_obj_get_int(args[1]);
        mp_machine_pwm_duty_set(self, duty);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_duty_obj, 1, 2, machine_pwm_duty);

// PWM.duty_u16([value])
STATIC mp_obj_t machine_pwm_duty_u16(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // Get duty cycle.
        return mp_machine_pwm_duty_get_u16(self);
    } else {
        // Set duty cycle.
        mp_int_t duty_u16 = mp_obj_get_int(args[1]);
        mp_machine_pwm_duty_set_u16(self, duty_u16);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_duty_u16_obj, 1, 2, machine_pwm_duty_u16);

// PWM.duty_ns([value])
STATIC mp_obj_t machine_pwm_duty_ns(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // Get duty cycle.
        return mp_machine_pwm_duty_get_ns(self);
    } else {
        // Set duty cycle.
        mp_int_t duty_ns = mp_obj_get_int(args[1]);
        mp_machine_pwm_duty_set_ns(self, duty_ns);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_duty_ns_obj, 1, 2, machine_pwm_duty_ns);


STATIC const mp_rom_map_elem_t machine_pwm_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&machine_pwm_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_pwm_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&machine_pwm_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty_u16), MP_ROM_PTR(&machine_pwm_duty_u16_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty_ns), MP_ROM_PTR(&machine_pwm_duty_ns_obj) },
    // constant: PWM OUT MODE
    { MP_ROM_QSTR(MP_QSTR_MODE_INDEPENDENT), MP_ROM_INT(PWM_OUT_MODE_INDEPENDENT) },
    { MP_ROM_QSTR(MP_QSTR_MODE_2SYNC), MP_ROM_INT(PWM_OUT_MODE_2SYNC) },
    { MP_ROM_QSTR(MP_QSTR_MODE_2COMPLEMENTARY), MP_ROM_INT(PWM_OUT_MODE_2COMPLEMENTARY) },
    { MP_ROM_QSTR(MP_QSTR_MODE_5SYNC), MP_ROM_INT(PWM_OUT_MODE_5SYNC) },
    // TODO: what's PWM_OUT_MODE_BREAK ?
    //{ MP_ROM_QSTR(MP_QSTR_PWM_OUT_MODE_BREAK), MP_ROM_INT(PWM_OUT_MODE_BREAK) },
    // constant: 
    { MP_ROM_QSTR(MP_QSTR_COUNTER_EDGE_ALIGNED_UP), MP_ROM_INT(PWM_COUNTERMODE_EDGEALIGNED_UP) },
    { MP_ROM_QSTR(MP_QSTR_COUNTER_EDGE_ALIGNED_DOWN), MP_ROM_INT(PWM_COUNTERMODE_EDGEALIGNED_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_COUNTER_CENTER_ALIGNED), MP_ROM_INT(PWM_COUNTERMODE_CENTERALIGNED) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pwm_locals_dict, machine_pwm_locals_dict_table);

const mp_obj_type_t machine_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = mp_machine_pwm_print,
    .make_new = mp_machine_pwm_make_new,
    .locals_dict = (mp_obj_dict_t *)&machine_pwm_locals_dict,
};
