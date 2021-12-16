#ifndef MICROPY_INCLUDED_XT804_MODMACHINE_H
#define MICROPY_INCLUDED_XT804_MODMACHINE_H

#include "py/obj.h"

typedef enum {
    MACHINE_RESET_REASON_PWRON = 1,
    MACHINE_RESET_REASON_HARD,
    MACHINE_RESET_REASON_WDT,
    MACHINE_RESET_REASON_DEEPSLEEP,
    MACHINE_RESET_REASON_SOFT,
} reset_reason_t;

typedef enum {
    MACHINE_WAKE_REASON_WLAN = 1,
    MACHINE_WAKE_REASON_PIN,
    MACHINE_WAKE_REASON_RTC,
    MACHINE_WAKE_REASON_TIMER
} wake_reason_t;

extern int8_t g_reset_reason;
extern int8_t g_wake_reason;


extern const mp_obj_type_t machine_timer_type;
extern const mp_obj_type_t machine_wdt_type;
extern const mp_obj_type_t machine_pin_type;
extern const mp_obj_type_t machine_touchpad_type;
extern const mp_obj_type_t machine_adc_type;
extern const mp_obj_type_t machine_dac_type;
extern const mp_obj_type_t machine_hw_i2c_type;
extern const mp_obj_type_t machine_hw_spi_type;
extern const mp_obj_type_t machine_i2s_type;
extern const mp_obj_type_t machine_uart_type;
extern const mp_obj_type_t machine_rtc_type;
extern const mp_obj_type_t machine_sdcard_type;
extern const mp_obj_type_t machine_pwm_type;
extern const mp_obj_type_t machine_spi_type;


void machine_init();
void machine_deinit();
void machine_pins_init();
void machine_pins_deinit();
void machine_timer_deinit_all();
void machine_i2s_init0();

#endif // MICROPY_INCLUDED_XT804_MODMACHINE_H
