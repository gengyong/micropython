

#define RUN_REPL 1

#if RUN_REPL

#include <wm_hal.h>
#include "mphalport.h"
#include "repl_uart0.h"

int main(void) {

    repl_listen_uart0();

    return 0;
}

//------------------------------
// IRQ vector
inline void DUMPGPIO(const GPIO_TypeDef * gpio) {
	printf("DATA: 0x%X\r\n", gpio->DATA);
	printf("DATA_B_EN: 0x%X\r\n", gpio->DATA_B_EN);
	printf("DIR: 0x%X\r\n", gpio->DIR);
	printf("PULLUP_EN: 0x%X\r\n", gpio->PULLUP_EN);
	printf("AF_SEL: 0x%X\r\n", gpio->AF_SEL);
	printf("AF_S1: 0x%X\r\n", gpio->AF_S1);
	printf("AF_S0: 0x%X\r\n", gpio->AF_S0);
	printf("PULLDOWN_EN: 0x%X\r\n", gpio->PULLDOWN_EN);
	printf("IS: 0x%X\r\n", gpio->IS);
	printf("IBE: 0x%X\r\n", gpio->IBE);
	printf("IEV: 0x%X\r\n", gpio->IEV);
	printf("IE: 0x%X\r\n", gpio->IE);
	printf("RIS: 0x%X\r\n", gpio->RIS);
	printf("MIS: 0x%X\r\n", gpio->MIS);
	printf("IC: 0x%X\r\n", gpio->IC);
}

#define readl(addr) ({unsigned int __v = (*(volatile unsigned int *) (addr)); __v;})
__attribute__((isr)) void CORET_IRQHandler(void)
{
    readl(0xE000E010);
    HAL_IncTick();
}

__attribute__((isr)) void GPIOA_IRQHandler(void)
{
    SET_BIT(GPIOA->IC, GPIOA->MIS);
    uint32_t pos = 0x0;
    while ((GPIOA->MIS >> pos) != 0) {
        if ((GPIOA->MIS) & (1uL << pos)) {
            HAL_GPIO_EXTI_Callback(GPIOA, pos);
        }
        pos++;
    }
}

__attribute__((isr)) void GPIOB_IRQHandler(void)
{
    SET_BIT(GPIOB->IC, GPIOB->MIS);
    uint32_t pos = 0x0;
    while ((GPIOB->MIS >> pos) != 0) {
        if ((GPIOB->MIS) & (1uL << pos)) {
            HAL_GPIO_EXTI_Callback(GPIOB, pos + 32);
        }
        pos++;
    }
}

__attribute__((isr)) void PMU_IRQHandler(void) {
    printf("PMU_IRQHandler");
    HAL_PMU_RTC_Callback(&xt804_rtc_source);
}

#else

/******************************************************************************
** 
 * \file        main.c
 * \author      IOsetting | iosetting@outlook.com
 * \date        
 * \brief       Demo code of PWM in 2-complementary mode
 * \note        This will drive 4 LEDs (3 onboard and 1 external) to show 2-complementary mode
 * \version     v0.1
 * \ingroup     demo
 * \remarks     test-board: HLK-W806-KIT-V1.0
 *              PWM Frequency = 40MHz / Prescaler / (Period + 1)；
 *              Duty Cycle(Edge Aligned)   = (Pulse + 1) / (Period + 1)
 *              Duty Cycle(Center Aligned) = (2 * Pulse + 1) / (2 * (Period + 1))
 *
 *              Connect PB3 to an external LED for PWM3 output
 *              PB3   -> ext LED1(-)
 *              3V3   -> ext LED1(+)(with 1KR resistor)
 *
******************************************************************************/

#include <stdio.h>
#include "wm_hal.h"

#define DUTY_MAX 100
#define DUTY_MIN 0
PWM_HandleTypeDef pwm[2]; 
int i, j, m[2] = {0}, d[2] = {DUTY_MIN, (DUTY_MIN + DUTY_MAX) / 2};

static void PWM_Init(PWM_HandleTypeDef *hpwm, uint32_t channel);
void Error_Handler(void);

int main(void)
{
    SystemClock_Config(CPU_CLK_160M);
    printf("enter main\r\n");

    PWM_Init(pwm + 1, PWM_CHANNEL_2);
    HAL_PWM_Start(pwm + 1);
    PWM_Init(pwm, PWM_CHANNEL_0);
    HAL_PWM_Start(pwm);

    while (1)
    {
        for (i = 0; i < 2; i++)
        {
            if (m[i] == 0) // Increasing
            {
                HAL_PWM_Duty_Set(pwm + i, d[i]++);
                if (d[i] == DUTY_MAX)
                {
                    m[i] = 1;
                }
            }
            else // Decreasing
            {
                HAL_PWM_Duty_Set(pwm + i, d[i]--);
                if (d[i] == DUTY_MIN)
                {
                    m[i] = 0;
                }
            }
        }
        HAL_Delay(20);
    }
}

static void PWM_Init(PWM_HandleTypeDef *hpwm, uint32_t channel)
{
    hpwm->Instance = PWM;
    hpwm->Init.AutoReloadPreload = PWM_AUTORELOAD_PRELOAD_ENABLE;
    hpwm->Init.CounterMode = PWM_COUNTERMODE_EDGEALIGNED_DOWN;
    hpwm->Init.Prescaler = 4;
    hpwm->Init.Period = 99;    // Frequency = 40,000,000 / 4 / (99 + 1) = 100,000 = 100KHz
    hpwm->Init.Pulse = 19;     // Duty Cycle = (19 + 1) / (99 + 1) = 20%
    hpwm->Init.OutMode = PWM_OUT_MODE_2COMPLEMENTARY; // 2-channel complementary mode
    hpwm->Channel = channel;   // Only Channel 0 and Channel 2 are allowed
    HAL_PWM_Init(hpwm);
}

void Error_Handler(void)
{
    while (1)
    {
    }
}

void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}


void HAL_MspInit(void)
{

}

void HAL_PWM_MspInit(PWM_HandleTypeDef *hpwm)
{
    __HAL_RCC_PWM_CLK_ENABLE();
    __HAL_AFIO_REMAP_PWM0(GPIOA, GPIO_PIN_10);
    __HAL_AFIO_REMAP_PWM1(GPIOB, GPIO_PIN_11);
    __HAL_AFIO_REMAP_PWM2(GPIOB, GPIO_PIN_12);
    __HAL_AFIO_REMAP_PWM3(GPIOB, GPIO_PIN_13);
}

void HAL_PWM_MspDeInit(PWM_HandleTypeDef *hpwm)
{
    __HAL_RCC_PWM_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);
}


#define readl(addr) ({unsigned int __v = (*(volatile unsigned int *) (addr)); __v;})
__attribute__((isr)) void CORET_IRQHandler(void)
{
    readl(0xE000E010);
    HAL_IncTick();
}



#endif