/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
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

#ifndef INCLUDED_XT804_MPHAL_H
#define INCLUDED_XT804_MPHAL_H

#include <wm_cpu.h>
#include <wm_gpio.h>

//========================================================
// abstruact HAL layer's implemention goes here


#define mp_hal_quiet_timing_enter()             MICROPY_BEGIN_ATOMIC_SECTION()
#define mp_hal_quiet_timing_exit(irq_state)     MICROPY_END_ATOMIC_SECTION(irq_state)

// void mp_hal_delay_ms(mp_uint_t ms);
#define mp_hal_delay_ms(ms)             HAL_Delay(ms)

// void mp_hal_delay_us(mp_uint_t us);
#define mp_hal_delay_us(us)             HAL_Delay((us)/1000)  //GengYong: TODO: buggy implemention
#define mp_hal_delay_us_fast(us)        HAL_Delay((us)/1000)

// mp_uint_t mp_hal_ticks_ms(void);
// HAL_TICK_FREQ_DEFAULT == 1kHz
#define mp_hal_ticks_ms()               HAL_GetTick()

// mp_uint_t mp_hal_ticks_us(void);
#define mp_hal_ticks_us()               (HAL_GetTick() * 1000) //GengYong: TODO: buggy implemention

// mp_uint_t mp_hal_ticks_cpu(void);
#define mp_hal_ticks_cpu()              HAL_GetTick()

// Nanoseconds since the Epoch.
// implemention goes xt804_mphal.c
uint64_t mp_hal_time_ns(void);

// uintptr_t mp_hal_stdio_poll(uintptr_t) 
#define mp_hal_stdio_poll(f)            (0) // no implemention

// below mp_hal_stdio_* functions was implemented in hal_uart0_core.c.
//   mp_hal_stdin_rx_chr
//   mp_hal_stdout_tx_strn
#undef mp_hal_stdin_rx_chr
#undef mp_hal_stdout_tx_strn
int mp_hal_stdin_rx_chr(void);
void mp_hal_stdout_tx_strn(const char *str, size_t len);


// below mp_hal_stdio_* functions was implement in shared/runtime/stdout_helpers.c
#undef mp_hal_stdout_tx_str
#undef mp_hal_stdout_tx_strn_cooked
extern void mp_hal_stdout_tx_str(const char *str);
extern void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);



//================================================
// Basic GPIO HAL abstract layer goes here.
#include "machine_pin.h"

#define MP_HAL_PIN_FMT                  "%q"
#define MP_HAL_PIN_MODE_INPUT           (GPIO_MODE_INPUT)
#define MP_HAL_PIN_MODE_OUTPUT          (GPIO_MODE_OUTPUT)
//? #define MP_HAL_PIN_MODE_ALT             (2) ?
//? #define MP_HAL_PIN_MODE_ANALOG          (3) ?
#if defined(GPIO_ASCR_ASC0)
//? #define MP_HAL_PIN_MODE_ADC             (11) ?
#else
//? #define MP_HAL_PIN_MODE_ADC             (3) ?
#endif
//? #define MP_HAL_PIN_MODE_OPEN_DRAIN      (5) ?
//? #define MP_HAL_PIN_MODE_ALT_OPEN_DRAIN  (6) ?
#define MP_HAL_PIN_PULL_NONE            (GPIO_NOPULL)
#define MP_HAL_PIN_PULL_UP              (GPIO_PULLUP)
#define MP_HAL_PIN_PULL_DOWN            (GPIO_PULLDOWN)
//? #define MP_HAL_PIN_SPEED_LOW            (GPIO_SPEED_FREQ_LOW)
//? #define MP_HAL_PIN_SPEED_MEDIUM         (GPIO_SPEED_FREQ_MEDIUM)
//? #define MP_HAL_PIN_SPEED_HIGH           (GPIO_SPEED_FREQ_HIGH)
//? #define MP_HAL_PIN_SPEED_VERY_HIGH      (GPIO_SPEED_FREQ_VERY_HIGH)

#define mp_hal_pin_obj_t        const machine_pin_obj_t *
#define mp_hal_get_pin_obj(o)   (o)
#define mp_hal_pin_name(p)      ((p)->name)
#define mp_hal_pin_input(p)     mp_hal_pin_config((p), MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_NONE, 0)
#define mp_hal_pin_output(p)    mp_hal_pin_config((p), MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0)
//++ GengYong: TODO
//#define mp_hal_pin_open_drain(p) mp_hal_pin_config((p), MP_HAL_PIN_MODE_OPEN_DRAIN, MP_HAL_PIN_PULL_NONE, 0)
#define mp_hal_pin_open_drain(p) mp_hal_pin_config((p), MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0)
#define mp_hal_pin_high(p)      HAL_GPIO_WritePin((p)->gpio, (p)->pin, GPIO_PIN_SET)
#define mp_hal_pin_low(p)       HAL_GPIO_WritePin((p)->gpio, (p)->pin, GPIO_PIN_RESET)
#define mp_hal_pin_od_low(p)    mp_hal_pin_low(p)
#define mp_hal_pin_od_high(p)   mp_hal_pin_high(p)
#define mp_hal_pin_read(p)      HAL_GPIO_ReadPin((p)->gpio, (p)->pin)
#define mp_hal_pin_write(p, v)  ((v) ? mp_hal_pin_high(p) : mp_hal_pin_low(p))

void mp_hal_gpio_clock_enable(GPIO_TypeDef *gpio);
void mp_hal_pin_config(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint32_t alt);
bool mp_hal_pin_config_alt(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint8_t fn, uint8_t unit);
void mp_hal_pin_config_speed(mp_hal_pin_obj_t pin_obj, uint32_t speed);

enum {
    MP_HAL_MAC_WLAN0 = 0,
    MP_HAL_MAC_WLAN1,
    MP_HAL_MAC_BDADDR,
    MP_HAL_MAC_ETH0,
};

void mp_hal_generate_laa_mac(int idx, uint8_t buf[6]);
void mp_hal_get_mac(int idx, uint8_t buf[6]);
void mp_hal_get_mac_ascii(int idx, size_t chr_off, size_t chr_len, char *dest);

//================================================
#include "shared/runtime/interrupt_char.h"












// END

// //===================

// #ifndef mp_hal_pin_obj_t
// #define mp_hal_pin_obj_t uint32_t
// #endif

// #define mp_hal_delay_us(us)                     HAL_Delay((us)/1000)
// #define mp_hal_delay_us_fast(us)                HAL_Delay((us)/1000)
// #define mp_hal_quiet_timing_enter()             MICROPY_BEGIN_ATOMIC_SECTION()
// #define mp_hal_quiet_timing_exit(irq_state)     MICROPY_END_ATOMIC_SECTION(irq_state)
// #define mp_hal_get_pin_obj(o)                   mp_obj_get_int(o)

// #include "py/ringbuf.h"
// #include "py/mphal.h"
// #include "shared/runtime/interrupt_char.h"





// The core that the MicroPython task(s) are pinned to.
// Until we move to IDF 4.2+, we need NimBLE on core 0, and for synchronisation
// with the ringbuffer and scheduler MP needs to be on the same core.
// See https://github.com/micropython/micropython/issues/5489
/*
#define MP_TASK_COREID (0)

extern TaskHandle_t mp_main_task_handle;

extern ringbuf_t stdin_ringbuf;

// Check the ESP-IDF error code and raise an OSError if it's not ESP_OK.
void check_esp_err(esp_err_t code);

uint32_t mp_hal_ticks_us(void);
__attribute__((always_inline)) static inline uint32_t mp_hal_ticks_cpu(void) {
    uint32_t ccount;
    #if CONFIG_IDF_TARGET_ESP32C3
    __asm__ __volatile__ ("csrr %0, 0x7E2" : "=r" (ccount)); // Machine Performance Counter Value
    #else
    __asm__ __volatile__ ("rsr %0,ccount" : "=a" (ccount));
    #endif
    return ccount;
}

void mp_hal_delay_us(uint32_t);
#define mp_hal_delay_us_fast(us) ets_delay_us(us)
void mp_hal_set_interrupt_char(int c);
uint32_t mp_hal_get_cpu_freq(void);

#define mp_hal_quiet_timing_enter() MICROPY_BEGIN_ATOMIC_SECTION()
#define mp_hal_quiet_timing_exit(irq_state) MICROPY_END_ATOMIC_SECTION(irq_state)

// Wake up the main task if it is sleeping
void mp_hal_wake_main_task_from_isr(void);

// C-level pin HAL
#include "py/obj.h"
#include "driver/gpio.h"
#define MP_HAL_PIN_FMT "%u"
#define mp_hal_pin_obj_t gpio_num_t
mp_hal_pin_obj_t machine_pin_get_id(mp_obj_t pin_in);
#define mp_hal_get_pin_obj(o) machine_pin_get_id(o)
#define mp_obj_get_pin(o) machine_pin_get_id(o) // legacy name; only to support esp8266/modonewire
#define mp_hal_pin_name(p) (p)
static inline void mp_hal_pin_input(mp_hal_pin_obj_t pin) {
    gpio_pad_select_gpio(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
}
static inline void mp_hal_pin_output(mp_hal_pin_obj_t pin) {
    gpio_pad_select_gpio(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT);
}
static inline void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin) {
    gpio_pad_select_gpio(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
}
static inline void mp_hal_pin_od_low(mp_hal_pin_obj_t pin) {
    gpio_set_level(pin, 0);
}
static inline void mp_hal_pin_od_high(mp_hal_pin_obj_t pin) {
    gpio_set_level(pin, 1);
}
static inline int mp_hal_pin_read(mp_hal_pin_obj_t pin) {
    return gpio_get_level(pin);
}
static inline void mp_hal_pin_write(mp_hal_pin_obj_t pin, int v) {
    gpio_set_level(pin, v);
}
*/



// mp_uint_t mp_hal_ticks_ms(void);
// void mp_hal_pin_input(mp_hal_pin_obj_t pin);
// void mp_hal_pin_output(mp_hal_pin_obj_t pin);
// void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin);
// void mp_hal_pin_od_high(mp_hal_pin_obj_t pin);

// void mp_hal_pin_write(mp_hal_pin_obj_t pin, int v);
// int mp_hal_pin_read(mp_hal_pin_obj_t pin);
// void mp_hal_pin_od_low(mp_hal_pin_obj_t pin);

#endif // INCLUDED_XT804_MPHAL_H
