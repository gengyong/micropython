
#ifndef MPY_XT804_HAL_BOOT_H_
#define MPY_XT804_HAL_BOOT_H_

// rtc clock
#include <wm_pmu.h>
extern PMU_HandleTypeDef xt804_rtc_source;

void mpy_hal_startup();
void mpy_hal_terminate();

#endif
