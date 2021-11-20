



#include "wm_hal.h"
#include "hal_uart0_core.h"
#include "mpy_hal_boot.h"

void mpy_hal_startup() {
    SystemClock_Config(CPU_CLK_240M);
    HAL_Init();
    HAL_UART0_Init();
}


void mpy_hal_terminate() {
    HAL_UART0_DeInit();
    HAL_DeInit();
}