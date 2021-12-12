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
#include "py/mperrno.h"
#include "extmod/machine_i2c.h"
#include "modmachine.h"

#include <wm_i2c.h>
#include "mphalport.h"

#define DEFAULT_I2C_FREQ (400000)


// TODO: waiting for hardware implemention.

typedef struct _machine_hw_i2c_obj_t {
    mp_obj_base_t base;
    I2C_HandleTypeDef i2c;
    uint32_t freq;
    uint8_t i2c_id;
} machine_hw_i2c_obj_t;

STATIC machine_hw_i2c_obj_t machine_hw_i2c_obj[] = {
    {{&machine_hw_i2c_type}, {0}, DEFAULT_I2C_FREQ, 0 },
};

STATIC void machine_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "I2C(%u, freq=%u)",
        self->i2c_id, self->freq);
}

mp_obj_t machine_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_freq, ARG_scl, ARG_sda };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = DEFAULT_I2C_FREQ} },
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    // Parse args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get I2C bus.
    int i2c_id = mp_obj_get_int(args[ARG_id].u_obj);
    if (i2c_id < 0 || i2c_id >= MP_ARRAY_SIZE(machine_hw_i2c_obj)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
    }

    mp_hal_pin_obj_t scl = NULL;
    mp_hal_pin_obj_t sda = NULL;
    // Set SCL/SDA pins if configured.
    if (args[ARG_scl].u_obj != mp_const_none) {
        mp_hal_pin_obj_t pin = mp_hal_get_pin_obj(args[ARG_scl].u_obj);
        if (pin->features & PIN_FEATURE_I2C_SCL) {
            scl = pin;
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("bad SCL pin"));
        } 
    }
    if (args[ARG_sda].u_obj != mp_const_none) {
        mp_hal_pin_obj_t pin = mp_hal_get_pin_obj(args[ARG_sda].u_obj);
        if (pin->features & PIN_FEATURE_I2C_SDA) {
            sda = pin;
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("bad SDA pin"));
        }
    }

     // Get static peripheral object.
    if (scl != NULL && sda != NULL) {
        machine_hw_i2c_obj_t *self = (machine_hw_i2c_obj_t *)&machine_hw_i2c_obj[i2c_id];
        self->freq = args[ARG_freq].u_int;

        // Initialise the I2C peripheral if any arguments given, or it was not initialised previously.
        if (n_args > 1 || n_kw > 0) {
            self->i2c.SCL_Port = scl->gpio;
            self->i2c.SCL_Pin = scl->pin;
            self->i2c.SDA_Port = sda->gpio;
            self->i2c.SDA_Pin = sda->pin;
            HAL_I2C_Init(&(self->i2c));
        }
        return MP_OBJ_FROM_PTR(self);
    }

    return mp_const_none;
}


//------------------------------------------
static inline void delay_us(void)
{
    uint32_t i;
    
    for (i = 0; i < (45 * 5); i++)
    {
        __NOP();
    }
}

static inline void I2C_Start(I2C_HandleTypeDef *hi2c)
{    
    I2C_SCL_H(hi2c);
    I2C_SDA_H(hi2c);
    delay_us();
    
    I2C_SDA_L(hi2c);
    delay_us();
    
    I2C_SCL_L(hi2c);
    delay_us();
}

static inline void I2C_Stop(I2C_HandleTypeDef *hi2c)
{
    I2C_SDA_L(hi2c);
    I2C_SCL_H(hi2c);
    delay_us();
    
    I2C_SDA_H(hi2c);
    delay_us();
}

static inline void I2C_Ack(I2C_HandleTypeDef *hi2c)
{    
    I2C_SDA_OUT(hi2c);
    I2C_SDA_L(hi2c);
    I2C_SCL_H(hi2c);
    delay_us();
    
    I2C_SCL_L(hi2c);
    delay_us();
    I2C_SDA_IN(hi2c);
}

static inline void I2C_Nack(I2C_HandleTypeDef *hi2c)
{
    I2C_SDA_OUT(hi2c);
    I2C_SDA_H(hi2c);
    I2C_SCL_H(hi2c);
    delay_us();
    
    I2C_SCL_L(hi2c);
    delay_us();
    I2C_SDA_IN(hi2c);
}

static inline uint8_t I2C_Wait_Ack(I2C_HandleTypeDef *hi2c)
{
    uint8_t ack;

    I2C_SDA_H(hi2c);
    delay_us();
    I2C_SDA_IN(hi2c);
    I2C_SCL_H(hi2c);
    delay_us();
    ack = I2C_SDA_GET(hi2c);
    I2C_SCL_L(hi2c);
    delay_us();
    I2C_SDA_OUT(hi2c);

    return ack;
}

static inline void I2C_Write_Byte(I2C_HandleTypeDef *hi2c, uint8_t data)
{
    int i;
    
    for (i = 0; i < 8; i ++)
    {
        if (data & 0x80)
        {
            I2C_SDA_H(hi2c);
        }
        else
        {
            I2C_SDA_L(hi2c);
        }
        I2C_SCL_H(hi2c);
        delay_us();
        I2C_SCL_L(hi2c);
        delay_us();
        data <<= 1;
    }
}

static inline uint8_t I2C_Read_Byte(I2C_HandleTypeDef *hi2c)
{
    uint8_t i, temp = 0;
    
    for (i = 0; i < 8; i++)
    {
        I2C_SCL_H(hi2c);
        delay_us();
        temp <<= 1;
        if (I2C_SDA_GET(hi2c))
        {
            temp |= 0x01;
        }
        I2C_SCL_L(hi2c);
        delay_us();
    }

    return temp;
}
//-----------------------------------------------------

int machine_hw_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    size_t remain_len = 0;
    for (size_t i = 0; i < n; ++i) {
        remain_len += bufs[i].len;
    }

    int num_acks = 0; // only valid for write; for read it'll be 0

    I2C_HandleTypeDef * hi2c = &(self->i2c);
    // start
    I2C_Start(hi2c);
    I2C_Write_Byte(hi2c, (addr & 0xFE));
    if (I2C_Wait_Ack(hi2c) == 0) {
        if (flags & MP_MACHINE_I2C_FLAG_READ) {
            for (; n--; ++bufs) {
                remain_len -= bufs->len;
                for (size_t i = 0; i < bufs->len; i++) {
                    bufs->buf[i] = I2C_Read_Byte(hi2c);
                    I2C_Ack(hi2c);
                }
            }
            I2C_Nack(hi2c);
            I2C_SDA_OUT(hi2c);
        } else {
            for (; n--; ++bufs) {
                remain_len -= bufs->len;
                for (size_t i = 0; i < bufs->len; i++)
                {
                    I2C_Write_Byte(hi2c, bufs->buf[i]);
                    if (I2C_Wait_Ack(hi2c) == 0) {
                        num_acks++;
                    } else {
                        goto OUT;
                    }
                }
            }
        }
    }

OUT:
    if (flags & MP_MACHINE_I2C_FLAG_STOP) {
        I2C_Stop(hi2c);
    }

    return num_acks;
}

STATIC const mp_machine_i2c_p_t machine_i2c_p = {
    .transfer = machine_hw_i2c_transfer,
};

const mp_obj_type_t machine_hw_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = machine_i2c_print,
    .make_new = machine_i2c_make_new,
    .protocol = &machine_i2c_p,
    .locals_dict = (mp_obj_dict_t *)&mp_machine_i2c_locals_dict,
};
