
#ifndef MPY_XT806_MACHINE_PIN_H
#define MPY_XT806_MACHINE_PIN_H

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
    mp_obj_base_t   base;
    pin_gpio_t *    gpio;
    uint32_t        pin;
    uint32_t        features;
    uint8_t         id;
    uint8_t         irq_slot;
} machine_pin_obj_t;

const machine_pin_obj_t* machine_pin_get_obj(mp_obj_t pin_in);

#define PIN_FEATURE_NONE        (0x0)
#define PIN_FEATURE_GPIO        ((uint32_t)0x00000001)
#define PIN_FEATURE_PWN0        ((uint32_t)0x00000002)
#define PIN_FEATURE_PWN1        ((uint32_t)0x00000004)
#define PIN_FEATURE_PWN2        ((uint32_t)0x00000008)
#define PIN_FEATURE_PWN3        ((uint32_t)0x00000010)
#define PIN_FEATURE_PWN4        ((uint32_t)0x00000020)
#define PIN_FEATURE_ADC0        ((uint32_t)0x00000040)
#define PIN_FEATURE_ADC1        ((uint32_t)0x00000080)
#define PIN_FEATURE_ADC2        ((uint32_t)0x00000100)
#define PIN_FEATURE_ADC3        ((uint32_t)0x00000200)
#define PIN_FEATURE_I2C_SCL     ((uint32_t)0x00000400)
#define PIN_FEATURE_I2C_SDA     ((uint32_t)0x00000800)
#define PIN_FEATURE_I2S_MCLK    ((uint32_t)0x00010000)


#define PIN_FEATURE_MASK_PWN (PIN_FEATURE_PWN0 | PIN_FEATURE_PWN1 | PIN_FEATURE_PWN2 | PIN_FEATURE_PWN3 | PIN_FEATURE_PWN4)
#define PIN_FEATURE_MASK_ADC (PIN_FEATURE_ADC0 | PIN_FEATURE_ADC1 | PIN_FEATURE_ADC2 | PIN_FEATURE_ADC3)


#endif // MPY_XT806_MACHINE_PIN_H

