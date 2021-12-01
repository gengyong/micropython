



#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>


#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "shared/runtime/pyexec.h"

#include "wm_hal.h"
#include "mpy_common.h"
#include "mpy_hal_boot.h"
#include "libc_port.h"
#include "hal_common.h"
#include "mphalport.h"
#include "repl_uart0.h"


//===========================================
#define DEBUG_SHOW_GCINFO 1
//===========================================

#ifndef HEAP_MEM_SIZE
//#define HEAP_MEM_SIZE ((288-8) * 1024)
#define HEAP_MEM_SIZE ((288-12) * 1024)
//#define HEAP_MEM_SIZE (283195)
#endif


#define V_SRAM_START  (0x20000000)
#define D_SRAM_START  (0x20000100)
#define D_SRAM_END    (0x20048000)  //Total RAM: 288KB

//extern uint32_t __ram_end;
//extern uint32_t __heap_end;

static uint32_t STACK_TOP;
static char HEAP_MEM[HEAP_MEM_SIZE];

inline uint32_t get_sp(void) {
    uint32_t result;
    __asm__ ("mov %0, sp\n" : "=r" (result));
    return result;
}

void gc_collect(void) {
    gc_collect_start();
    volatile uint32_t sp = get_sp();
    gc_collect_root((void **)sp, (STACK_TOP - sp) / sizeof(uint32_t));
    gc_collect_end();

#if DEBUG_SHOW_GCINFO
    //gc_dump_info();
    gc_info_t info;
    gc_info(&info);
    wm_printf(g_colorful_print ? 
        "\r\n\x1b[37;44m[GC]\x1b[0m\x1b[94m total: %u, used: %u, free: %u"
        : "\r\n[GC] total:%u, used: %u, free: %u",
        (uint)info.total, (uint)info.used, (uint)info.free);
    wm_printf(g_colorful_print ?
        "\r\n\x1b[37;44m[GC]\x1b[0m\x1b[94m No. of 1-blocks: %u, 2-blocks: %u, max blk sz: %u, max free sz: %u\x1b[0m\r\n"
        : "\r\n[GC] No. of 1-blocks: %u, 2-blocks: %u, max blk sz: %u, max free sz: %u\r\n",
        (uint)info.num_1block, (uint)info.num_2block, (uint)info.max_block, (uint)info.max_free);
#endif
}

static void session_startup() {
    mp_stack_set_top((void *)STACK_TOP);
    mp_stack_set_limit(STACK_TOP - V_SRAM_START);

    //mp_hal_init(); //TODO:

    gc_init(HEAP_MEM, HEAP_MEM + sizeof(HEAP_MEM));

    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_));
    mp_obj_list_init(mp_sys_argv, 0);

    #if MICROPY_MODULE_FROZEN
    pyexec_frozen_module("_boot.py");
    pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
        pyexec_file_if_exists("main.py");
    }
    #endif
}

static void session_terminate() {
   mp_deinit();
   //mp_hal_deinit(); //TODO:
}


void soft_reset(void) {
    gc_sweep_all();

    session_terminate();
    session_startup();

    mp_hal_stdout_tx_str("\r\nMPY: soft reboot\r\n");
    mp_hal_delay_ms(10); // allow UART to flush output
}

void repl_listen_uart0() {
    STACK_TOP = get_sp();

	//GPIO_InitTypeDef GPIO_InitStruct = { GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_OUTPUT, GPIO_NOPULL};
	//HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    //HAL_UART0_Init();
    
    mpy_hal_startup();
    session_startup();

    TLOG("STACK:  [0x%X - 0x%X] SIZE: 0x%X (%dKB)", D_SRAM_START, STACK_TOP, (STACK_TOP - D_SRAM_START), (STACK_TOP - D_SRAM_START)>>10);
    TLOG("HEAP:   [0x%p - 0x%p] SIZE: 0x%X (%dKB)", HEAP_MEM, HEAP_MEM + sizeof(HEAP_MEM), sizeof(HEAP_MEM), sizeof(HEAP_MEM)>>10);
    TLOG("SYSTEM: [0x%X - 0x%X, 0x%X - 0x%X] SIZE: 0x%X (%dKB)", 
        STACK_TOP, HEAP_MEM, 
        HEAP_MEM + sizeof(HEAP_MEM), D_SRAM_END, 
        (((uint32_t)HEAP_MEM - STACK_TOP) + (D_SRAM_END - ((uint32_t)HEAP_MEM + sizeof(HEAP_MEM)))),
        (((uint32_t)HEAP_MEM - STACK_TOP) + (D_SRAM_END - ((uint32_t)HEAP_MEM + sizeof(HEAP_MEM))))>>10);
    
    mp_hal_stdout_tx_str("\r\n");
    for (;;) {
        for (;;) {
            //TDEBUG("loop.");
            if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
                //TDEBUG("ready enter raw repl.");
                if (pyexec_raw_repl() != 0) {
                    //TDEBUG("raw repl force quit.");
                    soft_reset();
                    break;
                }
            } else {
                //TDEBUG("ready enter friendly repl.");
                if (pyexec_friendly_repl() != 0) {
                    //TDEBUG("friendly repl force quit.. reset.");
                    soft_reset();
                    break;
                }
            }
        }
    }

    TWARN("Quit.");
    mp_hal_delay_ms(10); // allow UART to flush output

    session_terminate();
    mpy_hal_terminate();

}

