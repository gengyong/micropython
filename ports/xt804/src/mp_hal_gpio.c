//================================================
// GPIO
#include "mp_hal_gpio.h"

void mp_hal_gpio_clock_enable(GPIO_TypeDef *gpio) {
    // #if defined(STM32L476xx) || defined(STM32L496xx)
    // if (gpio == GPIOG) {
    //     // Port G pins 2 thru 15 are powered using VddIO2 on these MCUs.
    //     HAL_PWREx_EnableVddIO2();
    // }
    // #endif

    // This logic assumes that all the GPIOx_EN bits are adjacent and ordered in one register

    // #if defined(STM32F0)
    // #define AHBxENR AHBENR
    // #define AHBxENR_GPIOAEN_Pos RCC_AHBENR_GPIOAEN_Pos
    // #elif defined(STM32F4) || defined(STM32F7)
    // #define AHBxENR AHB1ENR
    // #define AHBxENR_GPIOAEN_Pos RCC_AHB1ENR_GPIOAEN_Pos
    // #elif defined(STM32H7)
    // #define AHBxENR AHB4ENR
    // #define AHBxENR_GPIOAEN_Pos RCC_AHB4ENR_GPIOAEN_Pos
    // #elif defined(STM32L0)
    // #define AHBxENR IOPENR
    // #define AHBxENR_GPIOAEN_Pos RCC_IOPENR_IOPAEN_Pos
    // #elif defined(STM32L4) || defined(STM32WB)
    // #define AHBxENR AHB2ENR
    // #define AHBxENR_GPIOAEN_Pos RCC_AHB2ENR_GPIOAEN_Pos
    // #endif

    //uint32_t gpio_idx = ((uint32_t)gpio - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE);
    //RCC->AHBxENR |= 1 << (AHBxENR_GPIOAEN_Pos + gpio_idx);
    //volatile uint32_t tmp = RCC->AHBxENR; // Delay after enabling clock
    //(void)tmp;

    __HAL_RCC_GPIO_CLK_ENABLE();
}

void mp_hal_pin_config(mp_hal_pin_obj_t pin_obj, uint32_t mode, uint32_t pull, uint32_t alt) {
    GPIO_TypeDef *gpio = pin_obj->gpio;
    uint32_t pin = pin_obj->pin;

    mp_hal_gpio_clock_enable(gpio);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);

	//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_SET);
	//GPIO_InitStruct.Pin = GPIO_PIN_5;
	//GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	//GPIO_InitStruct.Pull = GPIO_PULLUP;
	//HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	//HAL_NVIC_SetPriority(GPIOB_IRQn, 0);
	//HAL_NVIC_EnableIRQ(GPIOB_IRQn);

    //--------------------------------------

    // mp_hal_gpio_clock_enable(gpio);
    // gpio->MODER = (gpio->MODER & ~(3 << (2 * pin))) | ((mode & 3) << (2 * pin));
    // #if defined(GPIO_ASCR_ASC0)
    // // The L4 has a special analog switch to connect the GPIO to the ADC
    // gpio->OTYPER = (gpio->OTYPER & ~(1 << pin)) | (((mode >> 2) & 1) << pin);
    // gpio->ASCR = (gpio->ASCR & ~(1 << pin)) | ((mode >> 3) & 1) << pin;
    // #else
    // gpio->OTYPER = (gpio->OTYPER & ~(1 << pin)) | ((mode >> 2) << pin);
    // #endif
    // gpio->OSPEEDR = (gpio->OSPEEDR & ~(3 << (2 * pin))) | (2 << (2 * pin)); // full speed
    // gpio->PUPDR = (gpio->PUPDR & ~(3 << (2 * pin))) | (pull << (2 * pin));
    // gpio->AFR[pin >> 3] = (gpio->AFR[pin >> 3] & ~(15 << (4 * (pin & 7)))) | (alt << (4 * (pin & 7)));
}

bool mp_hal_pin_config_alt(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint8_t fn, uint8_t unit) {
    // const pin_af_obj_t *af = pin_find_af(pin, fn, unit);
    // if (af == NULL) {
    //     return false;
    // }
    // mp_hal_pin_config(pin, mode, pull, af->idx);
    return true;
}

void mp_hal_pin_config_speed(mp_hal_pin_obj_t pin_obj, uint32_t speed) {
    // GPIO_TypeDef *gpio = pin_obj->gpio;
    // uint32_t pin = pin_obj->pin;
    // gpio->OSPEEDR = (gpio->OSPEEDR & ~(3 << (2 * pin))) | (speed << (2 * pin));
}
