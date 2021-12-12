/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Geng Yong (gengyong@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

//#include <time.h>
//#include <sys/time.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "shared/runtime/mpirq.h"
#include "shared/timeutils/timeutils.h"
#include "modmachine.h"
#include "mphalport.h"

#define RTC_ALARM_GROUP_CAPACITY (8)

#define UNIX_TIMESTAMP_AT_2000 (946684800)

PMU_HandleTypeDef xt804_rtc_source;

typedef struct _machine_rtc_obj_t {
    mp_obj_base_t base;
} machine_rtc_obj_t;

typedef struct _machine_rtc_irq_obj_t {
    mp_irq_obj_t base;
    uint8_t enabled;
    uint8_t trigger;
} machine_rtc_irq_obj_t;

typedef struct _machine_rtc_alarm_obj_t {
    mp_obj_base_t base;
    machine_rtc_irq_obj_t * irq;
    uint64_t alarm_datetime_s;
    uint32_t alarm_interval_ms;;
    uint16_t alarm_triggered_count;
} machine_rtc_alarm_obj_t;

typedef mp_uint_t (*mp_irq_switch_fun_t)(mp_obj_t self);
typedef struct _machine_rtc_irq_methods_t {
    mp_irq_methods_t base;
    mp_irq_switch_fun_t disable;
    mp_irq_switch_fun_t enable;
} machine_rtc_irq_methods_t;

STATIC const machine_rtc_irq_methods_t machine_rtc_irq_methods;

// RTC objects
const mp_obj_type_t machine_rtc_type;
STATIC const machine_rtc_obj_t machine_rtc_obj[1] = {
    { {&machine_rtc_type} }
};

const mp_obj_type_t machine_rtc_alarm_type;
static machine_rtc_alarm_obj_t machine_rtc_alarms_obj[MP_ARRAY_SIZE(machine_rtc_obj) * RTC_ALARM_GROUP_CAPACITY] = {
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 },
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 },
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 },
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 },
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 },
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 },
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 },
    { {&machine_rtc_alarm_type}, NULL, 0, 0, 0 }
};


/// The 8-tuple has the same format as CPython's datetime object:
///
///     (year, month, day, hours, minutes, seconds, milliseconds, tzinfo=None)
///
static inline void parse_datetime_tuple_strict(const mp_obj_t tuple, RTC_TimeTypeDef * datetime) {
    mp_obj_t *items = NULL;
    size_t len = 0;
    mp_obj_get_array(tuple, &len, &items);

    int year = 0, month = 0, date = 0, hours = 0, minutes = 0, seconds = 0, milliseconds = 0;
    
    if (len > 6) {
        milliseconds = mp_obj_get_int(items[6]);
        if (milliseconds < 0 || milliseconds > 1000) {
            mp_raise_ValueError(MP_ERROR_TEXT("Milliseconds: [0 - 1000]"));
        }
    }
    if (len > 5) {
        seconds = mp_obj_get_int(items[5]);
        if (seconds < 0 || seconds > 59) {
            mp_raise_ValueError(MP_ERROR_TEXT("Seconds: [0 - 59]"));
        }
    }
    if (len > 4) {
        minutes = mp_obj_get_int(items[4]);
        if (minutes < 0 || minutes > 59) {
            mp_raise_ValueError(MP_ERROR_TEXT("Minutes: [0 - 59]"));
        }
    }
    if (len > 3) {
        hours = mp_obj_get_int(items[3]);
        if (hours < 0 || hours > 23) {
            mp_raise_ValueError(MP_ERROR_TEXT("Hours: [0 - 23]"));
        }
    }
    if (len > 2) {
        date = mp_obj_get_int(items[2]);
        if (date < 1 || date > 31) {
            mp_raise_ValueError(MP_ERROR_TEXT("Date: [1 - 31]"));
        }
    }
    if (len > 1) {
        month = mp_obj_get_int(items[1]);
        if (month < 1 || month > 12) {
            mp_raise_ValueError(MP_ERROR_TEXT("Month: [1 - 12]"));
        }
    }
    if (len > 0) {
        year = mp_obj_get_int(items[0]);
        if (year < 0 || !IS_RTC_YEAR(year - 2000)) {
            mp_raise_ValueError(MP_ERROR_TEXT("Year: [2000 - 2099]"));
        }
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("datetime tuple is empty."));
    }

    int monthdays = timeutils_days_in_month(year, month);
    if (date > monthdays) {
        mp_raise_ValueError(MP_ERROR_TEXT("Date: the day not exists."));
    }
    datetime->Year = year - 2000;
    datetime->Month = month;
    datetime->Date = date;
    datetime->Hours = hours;
    datetime->Minutes = minutes;
    datetime->Seconds = seconds;
}

static inline void parse_datetime_tuple(const mp_obj_t tuple, RTC_TimeTypeDef * datetime) {
    mp_obj_t *items = NULL;
    size_t len = 0;
    mp_obj_get_array(tuple, &len, &items);

    if (len > 0) {
        int year = 0, month = 0, date = 0, hours = 0, minutes = 0, seconds = 0, milliseconds = 0;
        switch (len) {
        case 7: default:
            milliseconds = mp_obj_get_int(items[6]);
            milliseconds = milliseconds % 1000;
            seconds = milliseconds / 1000;
        case 6:
            seconds += mp_obj_get_int(items[5]);
            seconds = seconds % 60;
            minutes = seconds / 60;
        case 5:
            minutes += mp_obj_get_int(items[4]);
            minutes = minutes % 60;
            hours = minutes / 60;
        case 4:
            hours += mp_obj_get_int(items[3]);
            hours = hours % 24;
            date = hours / 24;
        case 3:
            date += (mp_obj_get_int(items[2]) - 1);
        case 2:
            month = (mp_obj_get_int(items[1]) - 1);
            month = month % 12;
            year = month / 12;
        case 1:
            year += mp_obj_get_int(items[0]);
        }

        while (date < 0) {
            date += timeutils_days_in_month(year, month);
            month++;
            if (month >= 12) {
                month -= 12;
                year++;
            }
        }

        int monthdays = 0;
        while ((monthdays = timeutils_days_in_month(year, month)) <= date) {
            date -= monthdays;
            month--;
            if (month < 0) {
                month += 12;
                year--;
            }
        }
        datetime->Year = year - 2000;
        datetime->Month = month + 1;
        datetime->Date = date + 1;
        datetime->Hours = hours;
        datetime->Minutes = minutes;
        datetime->Seconds = seconds;
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("datetime tuple is empty."));
    }
}


void schedule_rtc_alarm(const machine_rtc_obj_t * rtc) {
    RTC_TimeTypeDef rtctime;
    if (HAL_OK != HAL_PMU_RTC_GetTime(&xt804_rtc_source, &rtctime)) {
        return;
    }

    uint32_t seconds = timeutils_seconds_since_2000(
        rtctime.Year + 2000, 
        rtctime.Month,
        rtctime.Date,
        rtctime.Hours,
        rtctime.Minutes,
        rtctime.Seconds
    );

    uint32_t most_recent_datetime_s = UINT32_MAX;
    int most_recent_alarm_index = -1;
    int alarm_group_base_idx = MPY_ARRAY_INDEX(machine_rtc_obj, rtc) * RTC_ALARM_GROUP_CAPACITY;
    for (size_t alarm_index = alarm_group_base_idx; alarm_index < RTC_ALARM_GROUP_CAPACITY; alarm_index++) {
        machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);
        if (alarm->alarm_datetime_s > seconds) {
            if (alarm->alarm_datetime_s < most_recent_datetime_s) {
                most_recent_datetime_s = alarm->alarm_datetime_s;
                most_recent_alarm_index = alarm_index;
            }
        }
    }

    if (most_recent_alarm_index != -1) {
        timeutils_struct_time_t alarm_time;
        timeutils_seconds_since_2000_to_struct_time(most_recent_datetime_s, &alarm_time);
        if (alarm_time.tm_year < 2100) {
            RTC_TimeTypeDef rtctime = {
                .Year = alarm_time.tm_year - 2000,
                .Month = alarm_time.tm_mon,
                .Date = alarm_time.tm_mday,
                .Hours = alarm_time.tm_hour,
                .Minutes = alarm_time.tm_min,
                .Seconds = alarm_time.tm_sec
            };

            TDEBUG("schedule_rtc_alarm(%d): %d-%d-%d, %d:%d:%d", 
                most_recent_alarm_index,
                rtctime.Year, rtctime.Month, rtctime.Date,
                rtctime.Hours, rtctime.Minutes, rtctime.Seconds
                );

            HAL_NVIC_EnableIRQ(PMU_IRQn);
            HAL_PMU_RTC_Alarm_Enable(&xt804_rtc_source, &rtctime);
        }
    }
}

STATIC mp_obj_t machine_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    // return constant object
    return (mp_obj_t)&machine_rtc_obj;
}

STATIC mp_obj_t machine_rtc_datetime(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args <= 1) {
        RTC_TimeTypeDef datetime;
        int ret = HAL_PMU_RTC_GetTime(&xt804_rtc_source, &datetime);
        if (ret != HAL_OK) {
            mp_raise_OSError(MP_EIO);
        }
        
        mp_obj_t tuple[8] = {
            mp_obj_new_int(datetime.Year + 2000),
            mp_obj_new_int(datetime.Month),
            mp_obj_new_int(datetime.Date),
            mp_obj_new_int(datetime.Hours),
            mp_obj_new_int(datetime.Minutes),
            mp_obj_new_int(datetime.Seconds),
            mp_obj_new_int(0),
            mp_const_none
        };

        return mp_obj_new_tuple(8, tuple);
    } else {
        RTC_TimeTypeDef datetime;
        memset(&datetime, 0, sizeof(datetime));
        parse_datetime_tuple_strict(args[1], &datetime);
        if (HAL_OK != HAL_PMU_RTC_Start(&xt804_rtc_source, &datetime)) {
            mp_raise_OSError(MP_EINVAL);
        }

        mp_obj_t tuple[8] = {
            mp_obj_new_int(datetime.Year + 2000),
            mp_obj_new_int(datetime.Month),
            mp_obj_new_int(datetime.Date),
            mp_obj_new_int(datetime.Hours),
            mp_obj_new_int(datetime.Minutes),
            mp_obj_new_int(datetime.Seconds),
            mp_obj_new_int(0),
            mp_const_none
        };

        return mp_obj_new_tuple(8, tuple);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_datetime_obj, 1, 2, machine_rtc_datetime);


STATIC mp_obj_t machine_rtc_timestamp(mp_obj_t self_in) {
    RTC_TimeTypeDef rtctime;
    HAL_PMU_RTC_GetTime(&xt804_rtc_source, &rtctime);
    uint32_t seconds = timeutils_seconds_since_2000(
        rtctime.Year + 2000, 
        rtctime.Month,
        rtctime.Date,
        rtctime.Hours,
        rtctime.Minutes,
        rtctime.Seconds
    );
    return mp_obj_new_int_from_ll((uint64_t)seconds + UNIX_TIMESTAMP_AT_2000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_timestamp_obj, machine_rtc_timestamp);

STATIC mp_obj_t machine_rtc_alarm(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_id, ARG_time, ARG_handler, ARG_repeat };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,                        MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_time,                      MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_handler,                   MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_repeat,   MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} }
    };

    // parse args
    machine_rtc_obj_t *self = pos_args[0];

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), allowed_args, args);

    // check the alarm id
    uint alarm_id = args[ARG_id].u_int;
    if (alarm_id >= RTC_ALARM_GROUP_CAPACITY) {
        mp_raise_OSError(MP_ENODEV);
    }

    int alarm_index = MPY_ARRAY_INDEX(machine_rtc_obj, self) * RTC_ALARM_GROUP_CAPACITY + alarm_id;

    if (alarm_index > MP_ARRAY_SIZE(machine_rtc_alarms_obj)) {
        mp_raise_OSError(MP_ENODEV);
    }

    machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);

    if (n_args > 2) {
        if (mp_obj_is_type(args[ARG_time].u_obj, &mp_type_tuple)) {
            // datetime tuple given

            // repeat cannot be used with a datetime tuple
            if (args[ARG_repeat].u_bool) {
                mp_raise_ValueError(MP_ERROR_TEXT("repeat cannot be used with a datetime tuple"));
            }

            RTC_TimeTypeDef rtctime;
            memset(&rtctime, 0, sizeof(rtctime));
            parse_datetime_tuple(args[ARG_time].u_obj, &rtctime);

            alarm->alarm_triggered_count = 0;
            alarm->alarm_interval_ms = 0;
            alarm->alarm_datetime_s = timeutils_seconds_since_2000(
                rtctime.Year + 2000, 
                rtctime.Month,
                rtctime.Date,
                rtctime.Hours,
                rtctime.Minutes,
                rtctime.Seconds
            );

        } else { // then it must be an integer
            int milliseconds =  mp_obj_get_int(args[1].u_obj);
            if (milliseconds <= 0) {
                mp_raise_ValueError(MP_ERROR_TEXT("invalid time value"));
            }
            
            RTC_TimeTypeDef rtctime;
            if (HAL_OK != HAL_PMU_RTC_GetTime(&xt804_rtc_source, &rtctime)) {
                mp_raise_OSError(MP_EIO);
            }
            
            if (args[ARG_repeat].u_bool) {
                alarm->alarm_interval_ms = milliseconds;
            } else {
                alarm->alarm_interval_ms = 0;
            }
            alarm->alarm_triggered_count = 0;
            alarm->alarm_datetime_s = timeutils_seconds_since_2000(
                rtctime.Year + 2000, 
                rtctime.Month,
                rtctime.Date,
                rtctime.Hours,
                rtctime.Minutes,
                rtctime.Seconds
            );
            alarm->alarm_datetime_s += milliseconds / 1000;
        }

        if (args[ARG_handler].u_obj != mp_const_none) {
            // Allocate the IRQ object if it doesn't already exist.
            if (alarm->irq == NULL) {
                alarm->irq = m_new_obj(machine_rtc_irq_obj_t);
                alarm->irq->base.base.type = &mp_irq_type;
                alarm->irq->base.methods = (mp_irq_methods_t *)&machine_rtc_irq_methods;
                alarm->irq->base.parent = MP_OBJ_FROM_PTR(self); 
                alarm->irq->base.handler = mp_const_none;
                alarm->irq->base.ishard = false;
            }
            alarm->irq->base.handler = args[ARG_handler].u_obj;
            alarm->irq->trigger = alarm_index;
            alarm->irq->enabled = true;
        }

        schedule_rtc_alarm(self);
    }
    return (mp_obj_t)(alarm);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_rtc_alarm_obj, 1, machine_rtc_alarm);

STATIC mp_obj_t machine_rtc_alarm_left(size_t n_args, const mp_obj_t *args) {
    machine_rtc_obj_t *self = args[0];

    uint alarm_id = 0;
    if (n_args > 1) {
        alarm_id = mp_obj_get_int(args[1]);
    }
    if (alarm_id >= RTC_ALARM_GROUP_CAPACITY) {
        mp_raise_OSError(MP_ENODEV);
    }

    int alarm_index = MPY_ARRAY_INDEX(machine_rtc_obj, self) * RTC_ALARM_GROUP_CAPACITY + alarm_id;
    if (alarm_index > MP_ARRAY_SIZE(machine_rtc_alarms_obj)) {
        mp_raise_OSError(MP_ENODEV);
    }

    machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);

    if (alarm->alarm_datetime_s == 0) {
        return mp_obj_new_int(0);
    } else {
        RTC_TimeTypeDef rtctime;
        if (HAL_OK != HAL_PMU_RTC_GetTime(&xt804_rtc_source, &rtctime)) {
            mp_raise_OSError(MP_EIO);
        }

        int seconds = timeutils_seconds_since_2000(
            rtctime.Year + 2000, 
            rtctime.Month,
            rtctime.Date,
            rtctime.Hours,
            rtctime.Minutes,
            rtctime.Seconds
        );

        // calculate the ms left
        int ms_left = (alarm->alarm_datetime_s - seconds) * 1000;
        if (ms_left < 0) {
            ms_left = 0;
        }
        return mp_obj_new_int(ms_left);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_alarm_left_obj, 1, 2, machine_rtc_alarm_left);

STATIC mp_obj_t machine_rtc_alarm_cancel(size_t n_args, const mp_obj_t *args) {
    machine_rtc_obj_t *self = args[0];
    // only alarm id 0 is available
    int alarm_id = 0;
    if (n_args > 1) {
        alarm_id = mp_obj_get_int(args[1]);
    }
    if (alarm_id >= RTC_ALARM_GROUP_CAPACITY) {
        // support alarm 0 only now
        mp_raise_OSError(MP_ENODEV);
    }

    int alarm_index = MPY_ARRAY_INDEX(machine_rtc_obj, self) * RTC_ALARM_GROUP_CAPACITY + alarm_id;
    if (alarm_index > MP_ARRAY_SIZE(machine_rtc_alarms_obj)) {
        mp_raise_OSError(MP_ENODEV);
    }

    machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);

    // disable the alarm
    alarm->alarm_datetime_s = 0;
    m_del_obj(&mp_irq_type, alarm->irq);
    alarm->irq = NULL;

    schedule_rtc_alarm(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_alarm_cancel_obj, 1, 2, machine_rtc_alarm_cancel);

/// \method irq(trigger, priority, handler, wake)
STATIC mp_obj_t machine_rtc_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_trigger, ARG_handler, ARG_wake };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_trigger,                   MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_handler,                   MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_wake,     MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };

    machine_rtc_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), allowed_args, args);

   // save the power mode data for later
    // uint8_t pwrmode = (args[ARG_wake].u_obj == mp_const_none) ? PYB_PWR_MODE_ACTIVE : mp_obj_get_int(args[ARG_wake].u_obj);
    // if (pwrmode > (PYB_PWR_MODE_ACTIVE | PYB_PWR_MODE_LPDS | PYB_PWR_MODE_HIBERNATE)) {
    //     goto invalid_args;
    // }

    int alarm_id = args[ARG_trigger].u_int;
    if (alarm_id >= RTC_ALARM_GROUP_CAPACITY) {
        mp_raise_OSError(MP_ENODEV);
    }
    int alarm_index = MPY_ARRAY_INDEX(machine_rtc_obj, self) * RTC_ALARM_GROUP_CAPACITY + alarm_id;
    if (alarm_index > MP_ARRAY_SIZE(machine_rtc_alarms_obj)) {
        mp_raise_OSError(MP_ENODEV);
    }

    machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);
    //self->pwrmode = pwrmode;

    // Allocate the IRQ object if it doesn't already exist.
    if (alarm->irq == NULL) {
        alarm->irq = m_new_obj(machine_rtc_irq_obj_t);
        alarm->irq->base.base.type = &mp_irq_type;
        alarm->irq->base.methods = (mp_irq_methods_t *)&machine_rtc_irq_methods;
        alarm->irq->base.parent = MP_OBJ_FROM_PTR(alarm);
        alarm->irq->base.handler = mp_const_none;
        alarm->irq->base.ishard = false;
    }
    alarm->irq->base.handler = args[ARG_handler].u_obj;
    alarm->irq->trigger = alarm_index;
    alarm->irq->enabled = true;
    return alarm->irq;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_rtc_irq_obj, 1, machine_rtc_irq);


STATIC mp_obj_t machine_rtc_alarm_list(mp_obj_t self_in) {
    machine_rtc_obj_t * self = MP_OBJ_TO_PTR(self_in);
    int alarm_group = MPY_ARRAY_INDEX(machine_rtc_obj, self);
    int alarm_group_base_idx = alarm_group * RTC_ALARM_GROUP_CAPACITY;
    mp_obj_t list = mp_obj_new_list(0, NULL);
    for (size_t alarm_index = alarm_group_base_idx; alarm_index < RTC_ALARM_GROUP_CAPACITY; alarm_index++) {
        machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);
        if (alarm->alarm_datetime_s > 0) {
            mp_obj_list_append(list, MP_OBJ_FROM_PTR(alarm));
        }
    }
    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_alarm_list_obj, machine_rtc_alarm_list);


STATIC const mp_rom_map_elem_t machine_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime),        MP_ROM_PTR(&machine_rtc_datetime_obj) },
    // return unix timestamp in seconds(from 1970-01-01 00:00:00).
    { MP_ROM_QSTR(MP_QSTR_timestamp),       MP_ROM_PTR(&machine_rtc_timestamp_obj) },
    { MP_ROM_QSTR(MP_QSTR_alarm),           MP_ROM_PTR(&machine_rtc_alarm_obj) },
    { MP_ROM_QSTR(MP_QSTR_alarm_left),      MP_ROM_PTR(&machine_rtc_alarm_left_obj) },
    { MP_ROM_QSTR(MP_QSTR_alarm_cancel),    MP_ROM_PTR(&machine_rtc_alarm_cancel_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq),             MP_ROM_PTR(&machine_rtc_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_alarm_list),      MP_ROM_PTR(&machine_rtc_alarm_list_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_rtc_locals_dict, machine_rtc_locals_dict_table);

const mp_obj_type_t machine_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = machine_rtc_make_new,
    .locals_dict = (mp_obj_t)&machine_rtc_locals_dict,
};


STATIC void machine_rtc_alarm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_rtc_alarm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int id = MPY_ARRAY_INDEX(machine_rtc_alarms_obj, self) % RTC_ALARM_GROUP_CAPACITY;
    mp_printf(print, "Alarm(id=%d, time=%ld)", id, self->alarm_datetime_s + UNIX_TIMESTAMP_AT_2000);
}

STATIC mp_obj_t machine_rtc_alarm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    TDEBUG("machine_rtc_alarm_make_new");

    machine_rtc_obj_t * rtc = MP_OBJ_FROM_PTR(args[0]);

    int alarm_id = 0;
    if (n_args > 1) {
        alarm_id = mp_obj_get_int(args[1]);
    }
    if (alarm_id >= RTC_ALARM_GROUP_CAPACITY) {
        mp_raise_OSError(MP_ENODEV);
    }

    int alarm_index = MPY_ARRAY_INDEX(machine_rtc_obj, rtc) * RTC_ALARM_GROUP_CAPACITY + alarm_id;
    if (alarm_index > MP_ARRAY_SIZE(machine_rtc_alarms_obj)) {
        mp_raise_OSError(MP_ENODEV);
    }

    machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);
    return MP_OBJ_FROM_PTR(alarm);
}

// STATIC mp_obj_t machine_rtc_alarm_enable(mp_obj_t self_in) {
//     machine_rtc_alarm_obj_t *self = MP_OBJ_TO_PTR(self_in);
//     self->enabled = true;
//     return mp_obj_new_bool(self->enabled);
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_alarm_enable_obj,machine_rtc_alarm_enable);

// STATIC mp_obj_t machine_rtc_alarm_disable(mp_obj_t self_in) {
//     machine_rtc_alarm_obj_t *self = MP_OBJ_TO_PTR(self_in);
//     self->enabled = false;
//     return mp_obj_new_bool(self->enabled);
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_alarm_enable_obj, machine_rtc_alarm_enable);

// STATIC mp_obj_t machine_rtc_alarm_toggle(size_t n_args, const mp_obj_t *args)) {
//     if (n_args > 1) {
//         machine_rtc_alarm_obj_t *self = MP_OBJ_TO_PTR(args[0]);
//         if (n_args > 2) {
//             self->enable = mp_obj_get_bool(args[1]);
//         } else {
//             self->enable = self->enable ? false : true;
//         }
//         return mp_obj_new_bool(self->enabled);
//     }
//     return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_alarm_toggle_obj, 1, 2, machine_rtc_alarm_toggle);

STATIC mp_obj_t machine_rtc_alarm_alarm_cancel(mp_obj_t self_in) {
    machine_rtc_alarm_obj_t * alarm = MP_OBJ_TO_PTR(self_in);
    alarm->alarm_datetime_s = 0;
    m_del_obj(&mp_irq_type, alarm->irq);
    alarm->irq = NULL;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_alarm_alarm_cancel_obj, machine_rtc_alarm_alarm_cancel);

STATIC mp_obj_t machine_rtc_alarm_alarm_left(mp_obj_t self_in) {
   machine_rtc_alarm_obj_t *alarm = MP_OBJ_TO_PTR(self_in);
   if (alarm->alarm_datetime_s == 0) {
        return mp_obj_new_int(0);
    } else {
        RTC_TimeTypeDef rtctime;
        if (HAL_OK != HAL_PMU_RTC_GetTime(&xt804_rtc_source, &rtctime)) {
            mp_raise_OSError(MP_EIO);
        }

        int seconds = timeutils_seconds_since_2000(
            rtctime.Year + 2000, 
            rtctime.Month,
            rtctime.Date,
            rtctime.Hours,
            rtctime.Minutes,
            rtctime.Seconds
        );

        // calculate the ms left
        int ms_left = (alarm->alarm_datetime_s - seconds) * 1000;
        if (ms_left < 0) {
            ms_left = 0;
        }
        return mp_obj_new_int(ms_left);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_alarm_alarm_left_obj, machine_rtc_alarm_alarm_left);

STATIC mp_obj_t machine_rtc_alarm_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_wake };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler,                   MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_wake,     MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_obj = mp_const_none} },
    };

    machine_rtc_alarm_obj_t *alarm = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), allowed_args, args);

   // save the power mode data for later
    // uint8_t pwrmode = (args[ARG_wake].u_obj == mp_const_none) ? PYB_PWR_MODE_ACTIVE : mp_obj_get_int(args[ARG_wake].u_obj);
    // if (pwrmode > (PYB_PWR_MODE_ACTIVE | PYB_PWR_MODE_LPDS | PYB_PWR_MODE_HIBERNATE)) {
    //     goto invalid_args;
    // }

    // check the trigger
    // if (mp_obj_get_int(args[ARG_trigger].u_obj) != 0){
    //     mp_raise_OSError(MP_ENODEV);
    // }

    // int alarm_id = mp_obj_get_int(args[ARG_trigger]);
    // int alarm_index = MPY_ARRAY_INDEX(machine_rtc_obj, self) * RTC_ALARM_GROUP_CAPACITY + alarm_id;
    // if (alarm_index > MP_ARRAY_SIZE(machine_rtc_alarms_obj)) {
    //     mp_raise_OSError(MP_ENODEV);
    // }

    //machine_rtc_alarm_obj_t * self = MP_OBJ_TO_PTR(pos_args[0]);
    //self->pwrmode = pwrmode;

    if (n_args > 1) {
        // Allocate the IRQ object if it doesn't already exist.
        if (alarm->irq == NULL) {
            alarm->irq = m_new_obj(machine_rtc_irq_obj_t);
            alarm->irq->base.base.type = &mp_irq_type;
            alarm->irq->base.methods = (mp_irq_methods_t *)&machine_rtc_irq_methods;
            alarm->irq->base.parent = MP_OBJ_FROM_PTR(alarm);
            alarm->irq->base.handler = mp_const_none;
            alarm->irq->base.ishard = false;
        }
        alarm->irq->base.handler = args[ARG_handler].u_obj;
        alarm->irq->trigger = MPY_ARRAY_INDEX(machine_rtc_alarms_obj, alarm);
        alarm->irq->enabled = true;
        
    }
    return alarm->irq ? alarm->irq : mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_rtc_alarm_irq_obj, 1, machine_rtc_alarm_irq);

STATIC const mp_rom_map_elem_t machine_rtc_alarm_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_enable),      MP_ROM_PTR(&machine_rtc_alarm_enable_obj) },
    // { MP_ROM_QSTR(MP_QSTR_disable),     MP_ROM_PTR(&machine_rtc_alarm_disable_obj) },
    // { MP_ROM_QSTR(MP_QSTR_toggle),      MP_ROM_PTR(&machine_rtc_alarm_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_cancel),      MP_ROM_PTR(&machine_rtc_alarm_alarm_cancel_obj) },
    { MP_ROM_QSTR(MP_QSTR_left),        MP_ROM_PTR(&machine_rtc_alarm_alarm_left_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq),         MP_ROM_PTR(&machine_rtc_alarm_irq_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_rtc_alarm_locals_dict, machine_rtc_alarm_locals_dict_table);


const mp_obj_type_t machine_rtc_alarm_type = {
    { &mp_type_type },
    .name = MP_QSTR_Alarm,
    .make_new = machine_rtc_alarm_make_new,
    .print = machine_rtc_alarm_print,
    .locals_dict = (mp_obj_t)&machine_rtc_alarm_locals_dict,
};


STATIC mp_uint_t machine_rtc_alarm_irq_trigger(mp_obj_t self_in, mp_uint_t new_trigger) {
    // TODO: change trigger source.
    // machine_rtc_alarm_obj_t *alarm = MP_OBJ_TO_PTR(self_in);
    return 0;
}

STATIC mp_uint_t machine_rtc_alarm_irq_info(mp_obj_t self_in, mp_uint_t info_type) {
    machine_rtc_alarm_obj_t *alarm = MP_OBJ_TO_PTR(self_in);
    if (alarm->irq) {
        if (info_type == MP_IRQ_INFO_FLAGS) {
            return 0;
        } else if (info_type == MP_IRQ_INFO_TRIGGERS) {
            return alarm->irq->trigger;
        }
    }
    return 0;
}

STATIC mp_uint_t machine_rtc_alarm_irq_disable(mp_obj_t self_in) {
    machine_rtc_alarm_obj_t *alarm = MP_OBJ_TO_PTR(self_in);
    if (alarm->irq) {
        alarm->irq->enabled = false;
        return alarm->irq->enabled;
    }
    return 0;
}

STATIC mp_uint_t machine_rtc_alarm_irq_enable(mp_obj_t self_in) {
    machine_rtc_alarm_obj_t *alarm = MP_OBJ_TO_PTR(self_in);
    if (alarm->irq) {
        alarm->irq->enabled = true;
        return alarm->irq->enabled;
    }
    return 0;
}

STATIC const machine_rtc_irq_methods_t machine_rtc_irq_methods = {
    {
        .trigger = machine_rtc_alarm_irq_trigger,
        .info = machine_rtc_alarm_irq_info,
    },
    .disable = machine_rtc_alarm_irq_disable,
    .enable = machine_rtc_alarm_irq_enable
};

void HAL_PMU_RTC_Callback(PMU_HandleTypeDef * hpmu) {
    TDEBUG("HAL_PMU_RTC_Callback irq");
    
    if (g_wake_reason < 0) {
        g_wake_reason = MACHINE_WAKE_REASON_RTC;
    }

    RTC_TimeTypeDef rtctime;
    if (HAL_OK != HAL_PMU_RTC_GetTime(&xt804_rtc_source, &rtctime)) {
        return;
    }

    uint32_t seconds = timeutils_seconds_since_2000(
        rtctime.Year + 2000, 
        rtctime.Month,
        rtctime.Date,
        rtctime.Hours,
        rtctime.Minutes,
        rtctime.Seconds
    );

    const machine_rtc_obj_t * self = &machine_rtc_obj[0];

    int alarm_group = MPY_ARRAY_INDEX(machine_rtc_obj, self);
    int alarm_group_base_idx = alarm_group * RTC_ALARM_GROUP_CAPACITY;
    for (size_t alarm_index = alarm_group_base_idx; alarm_index < RTC_ALARM_GROUP_CAPACITY; alarm_index++) {
        machine_rtc_alarm_obj_t * alarm = &(machine_rtc_alarms_obj[alarm_index]);
        if (alarm->alarm_datetime_s > 0) {
            if (seconds >= alarm->alarm_datetime_s) {
                alarm->alarm_triggered_count++;
                if (alarm->irq && alarm->irq->enabled) {
                    TDEBUG("HAL_PMU_RTC_Callback call irq handler!");
                    mp_irq_handler(&alarm->irq->base);
                }
                if (alarm->alarm_interval_ms > 0) {
                    alarm->alarm_datetime_s += (alarm->alarm_interval_ms / 1000);
                }
            }
        }
    }

    schedule_rtc_alarm(self);
}


