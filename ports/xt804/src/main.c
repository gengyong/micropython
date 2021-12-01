

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

#include <stdio.h>                                            
#include "wm_hal.h"

void Error_Handler(void);
static void GPIO_Init(void);

static volatile uint8_t key_flag = 0;

int main(void)
{
    SystemClock_Config(CPU_CLK_160M);
    printf("enter main\r\n");
    HAL_Init();
    GPIO_Init();
    
    while (1)
    {
        if (key_flag == 1)
        {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_RESET)
            {
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
            }
            key_flag = 0;
        }
    }
    
    return 0;
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_SET);
    
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    HAL_NVIC_SetPriority(GPIOB_IRQn, 0);
    HAL_NVIC_EnableIRQ(GPIOB_IRQn);

}

void HAL_GPIO_EXTI_Callback(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    if ((GPIOx == GPIOB) && (GPIO_Pin == GPIO_PIN_5))
    {
        key_flag = 1;
    }
}


#define readl(addr) ({unsigned int __v = (*(volatile unsigned int *) (addr)); __v;})
__attribute__((isr)) void CORET_IRQHandler(void)
{
    readl(0xE000E010);
    HAL_IncTick();
}

__attribute__((isr)) void GPIOA_IRQHandler(void)
{
 	HAL_GPIO_EXTI_IRQHandler(GPIOA, GPIO_PIN_0);
}

__attribute__((isr)) void GPIOB_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIOB, GPIO_PIN_5);
}



#endif