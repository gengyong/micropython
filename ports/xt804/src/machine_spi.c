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
#include "extmod/machine_spi.h"
#include "modmachine.h"
#include "machine_pin.h"

#include <wm_hal.h>

#define DEFAULT_SPI_MODE        (SPI_MODE_MASTER)
#define DEFAULT_SPI_NSS         (SPI_NSS_SOFT)

#define DEFAULT_SPI_BAUDRATE    (20000000)
#define DEFAULT_SPI_POLARITY    (SPI_POLARITY_LOW)
#define DEFAULT_SPI_PHASE       (SPI_PHASE_1EDGE)
#define DEFAULT_SPI_BITS        (8)
#define DEFAULT_SPI_FIRSTBIT    (SPI_LITTLEENDIAN)

#ifndef MICROPY_HW_SPI0_SCK
#define MICROPY_HW_SPI0_SCK     (6)
#define MICROPY_HW_SPI0_MOSI    (7)
#define MICROPY_HW_SPI0_MISO    (4)
#endif



#define IS_VALID_PERIPH(spi, pin)   ((((pin) & 8) >> 3) == (spi))
#define IS_VALID_SCK(spi, pin)      (((pin) & 3) == 2 && IS_VALID_PERIPH(spi, pin))
#define IS_VALID_MOSI(spi, pin)     (((pin) & 3) == 3 && IS_VALID_PERIPH(spi, pin))
#define IS_VALID_MISO(spi, pin)     (((pin) & 3) == 0 && IS_VALID_PERIPH(spi, pin))

void spi_afio_mapping(GPIO_TypeDef *, uint32_t, uint32_t, uint32_t, uint32_t);

typedef struct _machine_spi_obj_t {
    mp_obj_base_t base;
    SPI_HandleTypeDef spi;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t firstbit;
    uint8_t sck;
    uint8_t mosi;
    uint8_t miso;
    uint32_t baudrate;
} machine_spi_obj_t;



STATIC machine_spi_obj_t machine_spi_obj[] = {
    {
        {&machine_spi_type}, 
        { 
            .Instance = SPI,
            .State = HAL_SPI_STATE_RESET,
        },
    },
};

STATIC void machine_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI(0, baudrate=%u, polarity=%u, phase=%u, bits=%u)",
        self->spi.Init.BaudRatePrescaler, 
        self->spi.Init.CLKPolarity,
        self->spi.Init.CLKPhase,
        self->spi.Init.FirstByte);
}

mp_obj_t machine_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_arg_check_num(n_args, n_kw, 1, 10, true);

    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_cs, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = DEFAULT_SPI_BAUDRATE} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_SPI_POLARITY} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_SPI_PHASE} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_SPI_BITS} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_SPI_FIRSTBIT} },
        { MP_QSTR_cs,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    TDEBUG("11111111111111111");
    // Parse the arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    TDEBUG("22222222222222222");
    // Get the SPI bus id.
    int spi_id = mp_obj_get_int(args[ARG_id].u_obj);
    if (spi_id < 0 || spi_id >= MP_ARRAY_SIZE(machine_spi_obj)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist"), spi_id);
    }

    // Get static peripheral object.
    machine_spi_obj_t *self = (machine_spi_obj_t *)&machine_spi_obj[spi_id];

    // Set SCK/MOSI/MISO pins if configured.
    const machine_pin_obj_t * cs = NULL;
    const machine_pin_obj_t * sck = NULL;
    const machine_pin_obj_t * mosi = NULL;
    const machine_pin_obj_t * miso = NULL;

    TDEBUG("333333333333333333:id=%d", spi_id);
    if (args[ARG_cs].u_obj != mp_const_none) {
        cs = mp_hal_get_pin_obj(args[ARG_cs].u_obj);
        if (cs && !(cs->features & PIN_FEATURE_SPI_CS)) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad cs pin"));
        }
    }


    TDEBUG("5555555555555555555555");
    if (args[ARG_sck].u_obj != mp_const_none) {
        sck = mp_hal_get_pin_obj(args[ARG_sck].u_obj);
        if (sck && !(sck->features & PIN_FEATURE_SPI_CLK)) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad SCK pin"));
        }
    }
    if (sck == NULL) {
        // TODO: fallback to default pin
        mp_raise_ValueError(MP_ERROR_TEXT("SCK pin not set."));
    }

    if (args[ARG_mosi].u_obj != mp_const_none) {
        mosi = mp_hal_get_pin_obj(args[ARG_mosi].u_obj);
        if (mosi && !(mosi->features & PIN_FEATURE_SPI_MOSI)) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad MOSI pin"));
        }
    }
    if (mosi == NULL) {
        // TODO: fallback to default pin
        mp_raise_ValueError(MP_ERROR_TEXT("MOSI pin not declare."));
    }

    if (args[ARG_miso].u_obj != mp_const_none) {
        miso = mp_hal_get_pin_obj(args[ARG_miso].u_obj);
        if (miso && !(miso->features & PIN_FEATURE_SPI_MISO)) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad MISO pin"));
        }
    }
    if (miso == NULL) {
        // TODO: fallback to default pin
        mp_raise_ValueError(MP_ERROR_TEXT("MISO pin not declare."));
    }
    TDEBUG("6666666666666666666");


    // Initialise the SPI peripheral if any arguments given, or it was not initialised previously.
    if (n_args > 1 || n_kw > 0) {
        if (true) {//mode == SPI_MODE_MASTER) {
            // #define SPI_BAUDRATEPRESCALER_2         (0x00000000U)	// 40M / 2 = 20M
            // #define SPI_BAUDRATEPRESCALER_4         (0x00000001U)	// 40M / 4 = 10M
            // #define SPI_BAUDRATEPRESCALER_8         (0x00000003U)	// 40M / 8 = 5M
            // #define SPI_BAUDRATEPRESCALER_10        (0x00000004U)	// 40M / 10 = 4M
            // #define SPI_BAUDRATEPRESCALER_20        (0x00000009U)	// 40M / 20 = 2M
            // #define SPI_BAUDRATEPRESCALER_40        (0x00000013U)	// 40M / 40 = 1M
            self->spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;//args[ARG_baudrate].u_int;
        } else {
            self->spi.Init.BaudRatePrescaler = 0;
        }
        self->spi.Init.CLKPolarity = args[ARG_polarity].u_int;
        self->spi.Init.CLKPhase = args[ARG_phase].u_int;
        //? = args[ARG_bits].u_int;
        self->spi.Init.FirstByte = args[ARG_firstbit].u_int;
        // if (self->firstbit == SPI_LSB_FIRST) {
        //     mp_raise_NotImplementedError(MP_ERROR_TEXT("LSB"));
        // }

        // spi_init(self->spi_inst, self->baudrate);
        // self->baudrate = spi_set_baudrate(self->spi_inst, self->baudrate);
        // spi_set_format(self->spi_inst, self->bits, self->polarity, self->phase, self->firstbit);
        // gpio_set_function(self->sck, GPIO_FUNC_SPI);
        // gpio_set_function(self->miso, GPIO_FUNC_SPI);
        // gpio_set_function(self->mosi, GPIO_FUNC_SPI);

        TDEBUG("7777777777777777777");

        self->spi.Init.Mode = DEFAULT_SPI_MODE;  // TODO:
        self->spi.Init.NSS = DEFAULT_SPI_NSS;    //TODO:
        if (self->spi.State == HAL_SPI_STATE_RESET) {
            self->spi.Lock = HAL_UNLOCKED;
            __HAL_RCC_SPI_CLK_ENABLE();
            if (cs) {
                __HAL_AFIO_REMAP_SPI_CS(cs->gpio, cs->pin);
            }
            __HAL_AFIO_REMAP_SPI_CLK(sck->gpio, sck->pin);
            __HAL_AFIO_REMAP_SPI_MISO(miso->gpio, miso->pin);
            __HAL_AFIO_REMAP_SPI_MOSI(mosi->gpio, mosi->pin);
            // TODO: init DMA here
        }
        self->spi.State = HAL_SPI_STATE_BUSY;

        if (HAL_SPI_Init(&(self->spi)) != HAL_OK) {
             mp_raise_OSError(EIO);
        }
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC void machine_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    };

    // Parse the arguments.
    machine_spi_obj_t *self = (machine_spi_obj_t *)self_in;
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    self->spi.Init.Mode = DEFAULT_SPI_MODE;  // TODO:
    self->spi.Init.NSS = DEFAULT_SPI_NSS;    //TODO:

    // Reconfigure the baudrate if requested.
    if (args[ARG_baudrate].u_int != -1) {
        mp_int_t baudrate = args[ARG_baudrate].u_int;
        self->spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_20; //TODO:
    }

    // Reconfigure the format if requested.
    bool set_format = false;
    if (args[ARG_polarity].u_int != -1) {
        mp_int_t polarity = args[ARG_polarity].u_int;
        self->spi.Init.CLKPolarity = DEFAULT_SPI_POLARITY;   //TODO:
        set_format = true;
    }
    if (args[ARG_phase].u_int != -1) {
        mp_int_t phase = args[ARG_phase].u_int;
        self->spi.Init.CLKPhase = DEFAULT_SPI_PHASE; //TODO:
        set_format = true;
    }
    if (args[ARG_bits].u_int != -1) {
        self->bits = args[ARG_bits].u_int;
        set_format = true;
    }
    if (args[ARG_firstbit].u_int != -1) {
        mp_int_t firstbit = args[ARG_firstbit].u_int;
        self->spi.Init.FirstByte = SPI_LITTLEENDIAN; //TODO:
    }
    if (set_format) {
        if (HAL_SPI_Init(&(self->spi)) != HAL_OK) {
             mp_raise_OSError(EIO);
        }
    }
}

STATIC void machine_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_spi_obj_t *self = (machine_spi_obj_t *)self_in;

    if (dest) {
        HAL_SPI_TransmitReceive(&(self->spi), src, dest, len, 0);
    } else {
        HAL_SPI_Transmit(&(self->spi), src, len, 0);
    }

    // TODO: non-blocking and DMA non-blocking mode
}

STATIC const mp_machine_spi_p_t machine_spi_p = {
    .init = machine_spi_init,
    .transfer = machine_spi_transfer,
};

const mp_obj_type_t machine_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = machine_spi_print,
    .make_new = machine_spi_make_new,
    .protocol = &machine_spi_p,
    .locals_dict = (mp_obj_dict_t *)&mp_machine_spi_locals_dict,
};
