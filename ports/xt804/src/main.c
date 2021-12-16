
#define RUN_REPL 1

#if RUN_REPL

#include <wm_hal.h>
#include "mphalport.h"
#include "modmachine.h"
#include "mpy_hal_boot.h"
#include "repl_uart0.h"

int main(void) {
    // check PMU status
    //TLOG("WDG->LD:%X", WDG->LD);
    //TLOG("WDG->VAL:%X", WDG->VAL);
    //TLOG("WDG->CR:%X", WDG->CR);
    //TLOG("WDG->CLR:%X", WDG->CLR);
    //TLOG("WDG->SRC:%X", WDG->SRC);
    //TLOG("WDG->STATE:%X", WDG->STATE);

    if (READ_BIT(PMU->IF, PMU_IF_STANDBY)) {
        g_reset_reason = MACHINE_RESET_REASON_DEEPSLEEP;
        //TDEBUG("BOOT: PMU_IF_STANDBY: 1 => reset reason: DeepSleep");
        if (READ_BIT(PMU->IF, PMU_IF_TIM0)) {
            g_wake_reason = MACHINE_WAKE_REASON_TIMER;
            //TDEBUG("BOOT: PMU_IF_TIM0: 1 => wake reason: TIMER");
        } else if (READ_BIT(PMU->IF, PMU_IF_IO_WAKE)) {
            g_wake_reason = MACHINE_WAKE_REASON_PIN;
            //TDEBUG("BOOT: PMU_IF_IO_WAKE: 1 => wake reason: PIN");
        } else if (READ_BIT(PMU->IF, PMU_IF_RTC)) {
            g_wake_reason = MACHINE_WAKE_REASON_RTC;
            //TDEBUG("BOOT: PMU_IF_RTC: 1 => wake reason: RTC");
        }
        SET_BIT(PMU->IF, PMU_IF_STANDBY | PMU_IF_TIM0 | PMU_IF_IO_WAKE | PMU_IF_RTC);
    } else {
        g_reset_reason = MACHINE_RESET_REASON_PWRON;
        //TDEBUG("BOOT: PMU_IF_STANDBY: 0, WDG_CLR:0 => reset reason: PowerOn");
    }
    // TODO: how to detect MACHINE_RESET_REASON_WDT?

    mpy_hal_startup();

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
__attribute__((isr)) void CORET_IRQHandler(void) {
    //printf("r:%ul\r\n", readl(0xE000E010));
    readl(0xE000E010);
    HAL_IncTick();
}

__attribute__((isr)) void GPIOA_IRQHandler(void) {
    SET_BIT(GPIOA->IC, GPIOA->MIS);
    uint32_t pos = 0x0;
    while ((GPIOA->MIS >> pos) != 0) {
        if ((GPIOA->MIS) & (1uL << pos)) {
            HAL_GPIO_EXTI_Callback(GPIOA, pos);
        }
        pos++;
    }
}

__attribute__((isr)) void GPIOB_IRQHandler(void) {
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
    TDEBUG("PMU_IRQHandler");

    uint32_t flag = READ_REG(xt804_rtc_source.Instance->IF);
	SET_BIT(xt804_rtc_source.Instance->IF, (PMU_IF_SLEEP | PMU_IF_STANDBY | PMU_IF_TIM0 | PMU_IF_IO_WAKE | PMU_IF_RTC));

	if ((flag & PMU_IF_TIM0) == PMU_IF_TIM0) {
		HAL_PMU_Tim0_Callback(&xt804_rtc_source);
	}
	if ((flag & PMU_IF_IO_WAKE) == PMU_IF_IO_WAKE) {
		HAL_PMU_IO_Callback(&xt804_rtc_source);
	}
	if ((flag & PMU_IF_RTC) == PMU_IF_RTC) {
		HAL_PMU_RTC_Callback(&xt804_rtc_source);
	}
}

__attribute__((isr)) void WDG_IRQHandler(void) {
    //WDG->CLR = WDG_CLR;
}

#else





#endif



