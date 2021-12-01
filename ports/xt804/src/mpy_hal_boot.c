





#include "mphalport.h"
#include "hal_uart0_core.h"
#include "mpy_hal_boot.h"

#include <wm_hal.h>

void mpy_hal_startup() {
    SystemClock_Config(CPU_CLK_240M);
    HAL_Init();
    HAL_UART0_Init();

    xt804_rtc_source.Instance = PMU;
    xt804_rtc_source.ClkSource = PMU_CLKSOURCE_32RC;
    HAL_PMU_Init(&xt804_rtc_source);

    RTC_TimeTypeDef time;
    time.Year = 10;
    time.Month = 3;
    time.Date = 31;
    time.Hours = 19;
    time.Minutes = 7;
    time.Seconds = 0;
    HAL_PMU_RTC_Start(&xt804_rtc_source, &time);
}


void mpy_hal_terminate() {
    HAL_PMU_DeInit(&xt804_rtc_source);

    HAL_UART0_DeInit();
    HAL_DeInit();
}