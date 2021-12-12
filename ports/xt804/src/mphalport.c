#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "py/obj.h"
#include "py/objstr.h"
#include "py/stream.h"
#include "py/mpstate.h"

#include "extmod/misc.h"
#include "shared/timeutils/timeutils.h"
#include "shared/runtime/pyexec.h"

#include "mphalport.h"

//================================================
// Pin
void mp_hal_pin_config(mp_hal_pin_obj_t pin_obj, uint32_t mode, uint32_t pull, uint32_t alt) {
    assert(alt == 0);
    GPIO_InitTypeDef initDef;
    initDef.Pin = pin_obj->pin;
    initDef.Mode = mode;
    initDef.Pull = pull;
    HAL_GPIO_Init(pin_obj->gpio, &initDef);
    mp_hal_gpio_clock_enable(pin_obj->gpio);
}

//================================================
// Time
#if 0 // GengYong: use xt804 timer.
void mp_hal_delay_us(uint32_t us) {
    // these constants are tested for a 240MHz clock
    const uint32_t this_overhead = 5;
    const uint32_t pend_overhead = 150;

    // return if requested delay is less than calling overhead
    if (us < this_overhead) {
        return;
    }
    us -= this_overhead;

    uint64_t t0 = esp_timer_get_time();
    for (;;) {
        uint64_t dt = esp_timer_get_time() - t0;
        if (dt >= us) {
            return;
        }
        if (dt + pend_overhead < us) {
            // we have enough time to service pending events
            // (don't use MICROPY_EVENT_POLL_HOOK because it also yields)
            mp_handle_pending(true);
        }
    }
}
#endif

uint64_t mp_hal_time_ns(void) {
    struct timeval tv;
    //gettimeofday(&tv, NULL);
    tv.tv_sec = 1577808000;
    tv.tv_usec = 0;
    uint64_t ns = tv.tv_sec * 1000000000ULL;
    ns += (uint64_t)tv.tv_usec * 1000ULL;
    return ns;
}

//================================================
// ringbuf for dupterm
//STATIC uint8_t stdin_ringbuf_array[260];
//ringbuf_t stdin_ringbuf = {stdin_ringbuf_array, sizeof(stdin_ringbuf_array), 0, 0};


//#include "usb.h"
//#include "usb_serial_jtag.h"

// TaskHandle_t mp_main_task_handle;

// STATIC uint8_t stdin_ringbuf_array[260];
// ringbuf_t stdin_ringbuf = {stdin_ringbuf_array, sizeof(stdin_ringbuf_array), 0, 0};

// Check the ESP-IDF error code and raise an OSError if it's not ESP_OK.
// void check_esp_err(esp_err_t code) {
//     if (code != ESP_OK) {
//         // map esp-idf error code to posix error code
//         uint32_t pcode = -code;
//         switch (code) {
//             case ESP_ERR_NO_MEM:
//                 pcode = MP_ENOMEM;
//                 break;
//             case ESP_ERR_TIMEOUT:
//                 pcode = MP_ETIMEDOUT;
//                 break;
//             case ESP_ERR_NOT_SUPPORTED:
//                 pcode = MP_EOPNOTSUPP;
//                 break;
//         }
//         // construct string object
//         mp_obj_str_t *o_str = m_new_obj_maybe(mp_obj_str_t);
//         if (o_str == NULL) {
//             mp_raise_OSError(pcode);
//             return;
//         }
//         o_str->base.type = &mp_type_str;
//         o_str->data = (const byte *)esp_err_to_name(code); // esp_err_to_name ret's ptr to const str
//         o_str->len = strlen((char *)o_str->data);
//         o_str->hash = qstr_compute_hash(o_str->data, o_str->len);
//         // raise
//         mp_obj_t args[2] = { MP_OBJ_NEW_SMALL_INT(pcode), MP_OBJ_FROM_PTR(o_str)};
//         nlr_raise(mp_obj_exception_make_new(&mp_type_OSError, 2, 0, args));
//     }
// }

// uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
//     uintptr_t ret = 0;
//     // if ((poll_flags & MP_STREAM_POLL_RD) && stdin_ringbuf.iget != stdin_ringbuf.iput) {
//     //     ret |= MP_STREAM_POLL_RD;
//     // }
//     return ret;
// }

// int mp_hal_stdin_rx_chr(void) {
//     for (;;) {
//         int c = ringbuf_get(&stdin_ringbuf);
//         if (c != -1) {
//             return c;
//         }
//         MICROPY_EVENT_POLL_HOOK
//         ulTaskNotifyTake(pdFALSE, 1);
//     }
// }

// void mp_hal_stdout_tx_strn(const char *str, size_t len) {
//     // Only release the GIL if many characters are being sent
//     bool release_gil = len > 20;
//     if (release_gil) {
//         MP_THREAD_GIL_EXIT();
//     }
//     #if CONFIG_USB_ENABLED
//     usb_tx_strn(str, len);
//     #elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
//     usb_serial_jtag_tx_strn(str, len);
//     #else
//     for (uint32_t i = 0; i < len; ++i) {
//         uart_tx_one_char(str[i]);
//     }
//     #endif
//     if (release_gil) {
//         MP_THREAD_GIL_ENTER();
//     }
//     mp_uos_dupterm_tx_strn(str, len);
// }

// uint32_t mp_hal_ticks_ms(void) {
//     return esp_timer_get_time() / 1000;
// }

// uint32_t mp_hal_ticks_us(void) {
//     return esp_timer_get_time();
// }

// void mp_hal_delay_ms(uint32_t ms) {
//     uint64_t us = ms * 1000;
//     uint64_t dt;
//     uint64_t t0 = esp_timer_get_time();
//     for (;;) {
//         mp_handle_pending(true);
//         MICROPY_PY_USOCKET_EVENTS_HANDLER
//         MP_THREAD_GIL_EXIT();
//         uint64_t t1 = esp_timer_get_time();
//         dt = t1 - t0;
//         if (dt + portTICK_PERIOD_MS * 1000 >= us) {
//             // doing a vTaskDelay would take us beyond requested delay time
//             taskYIELD();
//             MP_THREAD_GIL_ENTER();
//             t1 = esp_timer_get_time();
//             dt = t1 - t0;
//             break;
//         } else {
//             ulTaskNotifyTake(pdFALSE, 1);
//             MP_THREAD_GIL_ENTER();
//         }
//     }
//     if (dt < us) {
//         // do the remaining delay accurately
//         mp_hal_delay_us(us - dt);
//     }
// }

// void mp_hal_delay_us(uint32_t us) {
//     // these constants are tested for a 240MHz clock
//     const uint32_t this_overhead = 5;
//     const uint32_t pend_overhead = 150;

//     // return if requested delay is less than calling overhead
//     if (us < this_overhead) {
//         return;
//     }
//     us -= this_overhead;

//     uint64_t t0 = esp_timer_get_time();
//     for (;;) {
//         uint64_t dt = esp_timer_get_time() - t0;
//         if (dt >= us) {
//             return;
//         }
//         if (dt + pend_overhead < us) {
//             // we have enough time to service pending events
//             // (don't use MICROPY_EVENT_POLL_HOOK because it also yields)
//             mp_handle_pending(true);
//         }
//     }
// }

// uint64_t mp_hal_time_ns(void) {
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     uint64_t ns = tv.tv_sec * 1000000000ULL;
//     ns += (uint64_t)tv.tv_usec * 1000ULL;
//     return ns;
// }

// // Wake up the main task if it is sleeping
// void mp_hal_wake_main_task_from_isr(void) {
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     vTaskNotifyGiveFromISR(mp_main_task_handle, &xHigherPriorityTaskWoken);
//     if (xHigherPriorityTaskWoken == pdTRUE) {
//         portYIELD_FROM_ISR();
//     }
// }

// mp_uint_t mp_hal_ticks_ms(void) {
//     return 0;
// }


// void mp_hal_pin_input(mp_hal_pin_obj_t pin) {
//     // TODO:
// }

// void mp_hal_pin_output(mp_hal_pin_obj_t pin) {
//    // TODO:
// }

// void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin) {
//     // TODO:
// }

// void mp_hal_pin_od_high(mp_hal_pin_obj_t pin) {
//     // TODO:
//     //gpio_set_level(pin, 1);
// }

// void mp_hal_pin_write(mp_hal_pin_obj_t pin, int v) {
//     // TODO:
//     //gpio_set_level(pin, v);
// }

// int mp_hal_pin_read(mp_hal_pin_obj_t pin) {
//     //return gpio_get_level(pin);
//     // TODO:
//     return 0;
// }

// void mp_hal_pin_od_low(mp_hal_pin_obj_t pin) {
//     // TODO:
//     //gpio_set_level(pin, 0);
// }

