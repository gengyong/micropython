
#ifndef MPY_XT806MACHINE_PIN_H
#define MPY_XT806MACHINE_PIN_H

#include <wm_gpio.h>
#include "py/obj.h"

/*
typedef struct {
    mp_obj_base_t base;
    qstr name;
    uint32_t port   : 4;
    uint32_t pin    : 5;    // Some ARM processors use 32 bits/PORT
    uint32_t num_af : 4;
    uint32_t adc_channel : 5; // Some ARM processors use 32 bits/PORT
    uint32_t adc_num  : 3;  // 1 bit per ADC
    uint32_t pin_mask;
    pin_gpio_t *gpio;
    const pin_af_obj_t *af;
} pin_obj_t;
*/

typedef GPIO_TypeDef pin_gpio_t;

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    pin_gpio_t * gpio;
    uint32_t     pin;
    char         name[4];
    uint8_t      feature;
    uint8_t      id;
} machine_pin_obj_t;

typedef struct _machine_pin_irq_obj_t {
    mp_obj_base_t base;
    pin_gpio_t * gpio;
    uint32_t     pin;
    uint8_t      feature;
    uint8_t      id;
} machine_pin_irq_obj_t;



#endif // MPY_XT806MACHINE_PIN_H
