/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
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

#include <stdint.h>
#include <stdio.h>

//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "esp_sleep.h"
//#include "esp_pm.h"

#include <wm_hal.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "shared/runtime/pyexec.h"
#include "extmod/machine_bitstream.h"
#include "extmod/machine_mem.h"
#include "extmod/machine_signal.h"
#include "extmod/machine_pulse.h"
#include "extmod/machine_pwm.h"
#include "extmod/machine_i2c.h"
#include "extmod/machine_spi.h"
#include "modmachine.h"
//#include "machine_rtc.h"
#include "hal_common.h"

typedef enum {
    MP_PWRON_RESET = 1,
    MP_HARD_RESET,
    MP_WDT_RESET,
    MP_DEEPSLEEP_RESET,
    MP_SOFT_RESET
} reset_reason_t;

STATIC bool is_soft_reset = 0;

STATIC mp_obj_t machine_freq(size_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        // get
        wm_sys_clk clk;
        SystemClock_Get(&clk);
        return mp_obj_new_int(clk.cpuclk * 1000000);
    } else {
        // set
        mp_int_t freq = mp_obj_get_int(args[0]) / 1000000;
        if (freq != 40 && freq != 80 && freq != 120 && freq != 160 && freq != 240) {
            mp_raise_ValueError(MP_ERROR_TEXT("frequency must be 40MHz, 80Mhz, 120Mhz, 160MHz or 240MHz"));
        } else {
            TLOG("Set System Clock Freq to %uMhz", freq);
            uint32_t askclk = W805_PLL_CLK_MHZ / freq;
            SystemClock_Config(askclk);
        }
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_freq_obj, 0, 1, machine_freq);

STATIC mp_obj_t machine_sleep_helper(wake_type_t wake_type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    // enum {ARG_sleep_ms};
    // const mp_arg_t allowed_args[] = {
    //     { MP_QSTR_sleep_ms, MP_ARG_INT, { .u_int = 0 } },
    // };

    // mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    // mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    // mp_int_t expiry = args[ARG_sleep_ms].u_int;

    // if (expiry != 0) {
    //     esp_sleep_enable_timer_wakeup(((uint64_t)expiry) * 1000);
    // }

    // #if !CONFIG_IDF_TARGET_ESP32C3

    // if (machine_rtc_config.ext0_pin != -1 && (machine_rtc_config.ext0_wake_types & wake_type)) {
    //     esp_sleep_enable_ext0_wakeup(machine_rtc_config.ext0_pin, machine_rtc_config.ext0_level ? 1 : 0);
    // }

    // if (machine_rtc_config.ext1_pins != 0) {
    //     esp_sleep_enable_ext1_wakeup(
    //         machine_rtc_config.ext1_pins,
    //         machine_rtc_config.ext1_level ? ESP_EXT1_WAKEUP_ANY_HIGH : ESP_EXT1_WAKEUP_ALL_LOW);
    // }

    // if (machine_rtc_config.wake_on_touch) {
    //     if (esp_sleep_enable_touchpad_wakeup() != ESP_OK) {
    //         mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("esp_sleep_enable_touchpad_wakeup() failed"));
    //     }
    // }

    // #endif

    // switch (wake_type) {
    //     case MACHINE_WAKE_SLEEP:
    //         esp_light_sleep_start();
    //         break;
    //     case MACHINE_WAKE_DEEPSLEEP:
    //         esp_deep_sleep_start();
    //         break;
    // }
    return mp_const_none;
}

STATIC mp_obj_t machine_lightsleep(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    return machine_sleep_helper(MACHINE_WAKE_SLEEP, n_args, pos_args, kw_args);
};
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_lightsleep_obj, 0, machine_lightsleep);

STATIC mp_obj_t machine_deepsleep(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    return machine_sleep_helper(MACHINE_WAKE_DEEPSLEEP, n_args, pos_args, kw_args);
};
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_deepsleep_obj, 0,  machine_deepsleep);

STATIC mp_obj_t machine_reset_cause(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    if (is_soft_reset) {
        return MP_OBJ_NEW_SMALL_INT(MP_SOFT_RESET);
    }
    // switch (esp_reset_reason()) {
    //     case ESP_RST_POWERON:
    //     case ESP_RST_BROWNOUT:
    //         return MP_OBJ_NEW_SMALL_INT(MP_PWRON_RESET);
    //         break;

    //     case ESP_RST_INT_WDT:
    //     case ESP_RST_TASK_WDT:
    //     case ESP_RST_WDT:
    //         return MP_OBJ_NEW_SMALL_INT(MP_WDT_RESET);
    //         break;

    //     case ESP_RST_DEEPSLEEP:
    //         return MP_OBJ_NEW_SMALL_INT(MP_DEEPSLEEP_RESET);
    //         break;

    //     case ESP_RST_SW:
    //     case ESP_RST_PANIC:
    //     case ESP_RST_EXT: // Comment in ESP-IDF: "For ESP32, ESP_RST_EXT is never returned"
    //         return MP_OBJ_NEW_SMALL_INT(MP_HARD_RESET);
    //         break;

    //     case ESP_RST_SDIO:
    //     case ESP_RST_UNKNOWN:
    //     default:
    //         return MP_OBJ_NEW_SMALL_INT(0);
    //         break;
    // }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_reset_cause_obj, 0,  machine_reset_cause);

void machine_init(void) {
    is_soft_reset = 0;
}

void machine_deinit(void) {
    // we are doing a soft-reset so change the reset_cause
    is_soft_reset = 1;
}

// machine.info([dump_alloc_table])
// Print out lots of information about the board.
STATIC mp_obj_t machine_info(size_t n_args, const mp_obj_t *args) {
    // get and print unique id; 96 bits
    {
        //byte *id = (byte *)MP_HAL_UNIQUE_ID_ADDRESS;
        //printf("ID=%02x%02x%02x%02x:%02x%02x%02x%02x:%02x%02x%02x%02x\n", id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7], id[8], id[9], id[10], id[11]);
    }

    //printf("DEVID=0x%04x\nREVID=0x%04x\n", (unsigned int)HAL_GetDEVID(), (unsigned int)HAL_GetREVID());

    // get and print clock speeds
    // SYSCLK=168MHz, HCLK=168MHz, PCLK1=42MHz, PCLK2=84MHz
    {
        // printf("S=%u\nH=%u\nP1=%u\nP2=%u\n",
        //     (unsigned int)HAL_RCC_GetSysClockFreq(),
        //     (unsigned int)HAL_RCC_GetHCLKFreq(),
        //     (unsigned int)HAL_RCC_GetPCLK1Freq(),
        //     (unsigned int)HAL_RCC_GetPCLK2Freq());
    }

    // to print info about memory
    {
        // printf("_etext=%p\n", &_etext);
        // printf("_sidata=%p\n", &_sidata);
        // printf("_sdata=%p\n", &_sdata);
        // printf("_edata=%p\n", &_edata);
        // printf("_sbss=%p\n", &_sbss);
        // printf("_ebss=%p\n", &_ebss);
        // printf("_sstack=%p\n", &_sstack);
        // printf("_estack=%p\n", &_estack);
        // printf("_ram_start=%p\n", &_ram_start);
        // printf("_heap_start=%p\n", &_heap_start);
        // printf("_heap_end=%p\n", &_heap_end);
        // printf("_ram_end=%p\n", &_ram_end);
    }

    // qstr info
    {
        size_t n_pool, n_qstr, n_str_data_bytes, n_total_bytes;
        qstr_pool_info(&n_pool, &n_qstr, &n_str_data_bytes, &n_total_bytes);
        printf("qstr:\n  n_pool=%u\n  n_qstr=%u\n  n_str_data_bytes=%u\n  n_total_bytes=%u\n", n_pool, n_qstr, n_str_data_bytes, n_total_bytes);
    }

    // GC info
    {
        gc_info_t info;
        gc_info(&info);
        printf("GC:\n");
        printf("  %u total\n", info.total);
        printf("  %u : %u\n", info.used, info.free);
        printf("  1=%u 2=%u m=%u\n", info.num_1block, info.num_2block, info.max_block);
    }

    // free space on flash
    {
        #if MICROPY_VFS_FAT
        for (mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table); vfs != NULL; vfs = vfs->next) {
            if (strncmp("/flash", vfs->str, vfs->len) == 0) {
                // assumes that it's a FatFs filesystem
                fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
                DWORD nclst;
                f_getfree(&vfs_fat->fatfs, &nclst);
                printf("LFS free: %u bytes\n", (uint)(nclst * vfs_fat->fatfs.csize * 512));
                break;
            }
        }
        #endif
    }

    #if MICROPY_PY_THREAD
    //pyb_thread_dump();
    #endif

    if (n_args == 1) {
        // arg given means dump gc allocation table
        gc_dump_alloc_table();
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_info_obj, 0, 1, machine_info);

STATIC mp_obj_t machine_wake_reason(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    //return MP_OBJ_NEW_SMALL_INT(esp_sleep_get_wakeup_cause());
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_wake_reason_obj, 0,  machine_wake_reason);

STATIC mp_obj_t machine_reset(void) {
    //esp_restart();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

STATIC mp_obj_t machine_soft_reset(void) {
    pyexec_system_exit = PYEXEC_FORCED_EXIT;
    mp_raise_type(&mp_type_SystemExit);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_soft_reset_obj, machine_soft_reset);

STATIC mp_obj_t machine_unique_id(void) {
    uint8_t id[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    return mp_obj_new_bytes(id, sizeof(id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_unique_id_obj, machine_unique_id);

STATIC mp_obj_t machine_idle(void) {
    // MP_THREAD_GIL_EXIT();
    // taskYIELD();
    // MP_THREAD_GIL_ENTER();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_idle_obj, machine_idle);

STATIC mp_obj_t machine_disable_irq(void) {
    //uint32_t state = MICROPY_BEGIN_ATOMIC_SECTION();
    //return mp_obj_new_int(state);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_disable_irq_obj, machine_disable_irq);

STATIC mp_obj_t machine_enable_irq(mp_obj_t state_in) {
   // uint32_t state = mp_obj_get_int(state_in);
   // MICROPY_END_ATOMIC_SECTION(state);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_enable_irq_obj, machine_enable_irq);

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_machine) },
    { MP_ROM_QSTR(MP_QSTR_unique_id), MP_ROM_PTR(&machine_unique_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&machine_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset), MP_ROM_PTR(&machine_soft_reset_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&machine_reset_obj) },

    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_freq_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&machine_lightsleep_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_lightsleep), MP_ROM_PTR(&machine_lightsleep_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_deepsleep), MP_ROM_PTR(&machine_deepsleep_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_idle), MP_ROM_PTR(&machine_idle_obj) },

    //{ MP_ROM_QSTR(MP_QSTR_disable_irq), MP_ROM_PTR(&machine_disable_irq_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_enable_irq), MP_ROM_PTR(&machine_enable_irq_obj) },

    #if MICROPY_PY_MACHINE_BITSTREAM
    { MP_ROM_QSTR(MP_QSTR_bitstream), MP_ROM_PTR(&machine_bitstream_obj) },
    #endif
    #if MICROPY_PY_MACHINE_PULSE
    //{ MP_ROM_QSTR(MP_QSTR_time_pulse_us), MP_ROM_PTR(&machine_time_pulse_us_obj) },
    #endif

    { MP_ROM_QSTR(MP_QSTR_mem8), MP_ROM_PTR(&machine_mem8_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem16), MP_ROM_PTR(&machine_mem16_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem32), MP_ROM_PTR(&machine_mem32_obj) },


    //{ MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&machine_timer_type) },
    //{ MP_ROM_QSTR(MP_QSTR_WDT), MP_ROM_PTR(&machine_wdt_type) },
    #if MICROPY_HW_ENABLE_SDCARD
    //{ MP_ROM_QSTR(MP_QSTR_SDCard), MP_ROM_PTR(&machine_sdcard_type) },
    #endif

    // wake abilities
    //{ MP_ROM_QSTR(MP_QSTR_SLEEP), MP_ROM_INT(MACHINE_WAKE_SLEEP) },
    //{ MP_ROM_QSTR(MP_QSTR_DEEPSLEEP), MP_ROM_INT(MACHINE_WAKE_DEEPSLEEP) },
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },
    { MP_ROM_QSTR(MP_QSTR_Signal), MP_ROM_PTR(&machine_signal_type) },
    
    { MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&machine_adc_type) },
    #if MICROPY_PY_MACHINE_DAC
    { MP_ROM_QSTR(MP_QSTR_DAC), MP_ROM_PTR(&machine_dac_type) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&machine_hw_i2c_type) },
    { MP_ROM_QSTR(MP_QSTR_SoftI2C), MP_ROM_PTR(&mp_machine_soft_i2c_type) },
    #if MICROPY_PY_MACHINE_I2S
    { MP_ROM_QSTR(MP_QSTR_I2S), MP_ROM_PTR(&machine_i2s_type) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_PWM), MP_ROM_PTR(&machine_pwm_type) },
    { MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&machine_rtc_type) },
    //{ MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&machine_hw_spi_type) },
    //{ MP_ROM_QSTR(MP_QSTR_SoftSPI), MP_ROM_PTR(&mp_machine_soft_spi_type) },
    //{ MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&machine_uart_type) },

    // Reset reasons
    //{ MP_ROM_QSTR(MP_QSTR_reset_cause), MP_ROM_PTR(&machine_reset_cause_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_HARD_RESET), MP_ROM_INT(MP_HARD_RESET) },
    //{ MP_ROM_QSTR(MP_QSTR_PWRON_RESET), MP_ROM_INT(MP_PWRON_RESET) },
    //{ MP_ROM_QSTR(MP_QSTR_WDT_RESET), MP_ROM_INT(MP_WDT_RESET) },
    //{ MP_ROM_QSTR(MP_QSTR_DEEPSLEEP_RESET), MP_ROM_INT(MP_DEEPSLEEP_RESET) },
    //{ MP_ROM_QSTR(MP_QSTR_SOFT_RESET), MP_ROM_INT(MP_SOFT_RESET) },

    // Wake reasons
    //{ MP_ROM_QSTR(MP_QSTR_wake_reason), MP_ROM_PTR(&machine_wake_reason_obj) },
    // { MP_ROM_QSTR(MP_QSTR_PIN_WAKE), MP_ROM_INT(0) },
    // { MP_ROM_QSTR(MP_QSTR_EXT0_WAKE), MP_ROM_INT(1) },
    //{ MP_ROM_QSTR(MP_QSTR_EXT1_WAKE), MP_ROM_INT(2) },
    //{ MP_ROM_QSTR(MP_QSTR_TIMER_WAKE), MP_ROM_INT(3) },
    //{ MP_ROM_QSTR(MP_QSTR_TOUCHPAD_WAKE), MP_ROM_INT(4) },
    // { MP_ROM_QSTR(MP_QSTR_ULP_WAKE), MP_ROM_INT(5) },

    

    
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&machine_module_globals,
};

