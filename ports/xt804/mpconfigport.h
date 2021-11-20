#include <stdint.h>
#include <unistd.h>

// options to control how MicroPython is built

// Use the minimal starting configuration (disables all optional features).
//#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)
//#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES)
//#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)
//#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_FULL_FEATURES)
//#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EVERYTHING)

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
#define MICROPY_ENABLE_COMPILER     (1)

// type definitions for the specific machine
typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

typedef int32_t __int32_t;
typedef uint32_t __uint32_t;
typedef int16_t __int16_t;
typedef uint16_t __uint16_t;
typedef int8_t __int8_t;
typedef uint8_t __uint8_t;

// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MICROPY_HW_BOARD_NAME "W806"
#define MICROPY_HW_MCU_NAME "XT804"

#define MICROPY_REPL_EVENT_DRIVEN         (0)

#define MICROPY_OBJ_REPR (MICROPY_OBJ_REPR_A)
#define MICROPY_ENABLE_EXTERNAL_IMPORT    (1)
#define MICROPY_GCREGS_SETJMP             (1)
#define MICROPY_ALLOC_PATH_MAX            (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT    (16)

// emitters
#define MICROPY_PERSISTENT_CODE_LOAD        (1)

// compiler configuration
#define MICROPY_COMP_MODULE_CONST           (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN    (1)

// optimisations
#define MICROPY_OPT_COMPUTED_GOTO           (1)
#define MICROPY_OPT_LOAD_ATTR_FAST_PATH     (1)
#define MICROPY_OPT_MAP_LOOKUP_CACHE        (1)
#define MICROPY_OPT_MPZ_BITWISE             (1)

// Python internal features
#define MICROPY_READER_VFS                  (1)
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_ENABLE_FINALISER            (1)
#define MICROPY_STACK_CHECK                 (1)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_KBD_EXCEPTION               (1)
#define MICROPY_HELPER_REPL                 (1)
#define MICROPY_REPL_EMACS_KEYS             (1)
#define MICROPY_REPL_AUTO_INDENT            (1)
#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_ENABLE_SOURCE_LINE          (1)
#define MICROPY_ERROR_REPORTING             (MICROPY_ERROR_REPORTING_NORMAL)
#define MICROPY_WARNINGS                    (1)
#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_CPYTHON_COMPAT              (1)
#define MICROPY_STREAMS_NON_BLOCK           (1)
#define MICROPY_STREAMS_POSIX_API           (1)
#define MICROPY_MODULE_BUILTIN_INIT         (1)
#define MICROPY_MODULE_WEAK_LINKS           (1)
#define MICROPY_QSTR_EXTRA_POOL             mp_qstr_frozen_const_pool
#define MICROPY_CAN_OVERRIDE_BUILTINS       (1)
#define MICROPY_USE_INTERNAL_ERRNO          (0) // errno.h from xtensa-esp32-elf/sys-include/sys
#define MICROPY_USE_INTERNAL_PRINTF         (0) // ESP32 SDK requires its own printf
#define MICROPY_ENABLE_SCHEDULER            (1)
#define MICROPY_SCHEDULER_DEPTH             (8)
#define MICROPY_VFS                         (1)
#define MICROPY_VFS_FAT                     (1)
#define MICROPY_VFS_POSIX_FILE              (0)
#define MICROPY_PY_SYS_PLATFORM             "mpy-xt804"

// control over Python builtins
#define MICROPY_PY_FUNCTION_ATTRS           (1)
#define MICROPY_PY_DESCRIPTORS              (1)
#define MICROPY_PY_DELATTR_SETATTR          (1)
#define MICROPY_PY_FSTRINGS                 (1)
#define MICROPY_PY_STR_BYTES_CMP_WARN       (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE     (1)
#define MICROPY_PY_BUILTINS_STR_CENTER      (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION   (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES  (1)
#define MICROPY_PY_BUILTINS_BYTEARRAY       (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW      (1)
#define MICROPY_PY_BUILTINS_SET             (1)
#define MICROPY_PY_BUILTINS_SLICE           (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_SLICE_INDICES   (1)
#define MICROPY_PY_BUILTINS_FROZENSET       (1)
#define MICROPY_PY_BUILTINS_PROPERTY        (1)
#define MICROPY_PY_BUILTINS_RANGE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_ROUND_INT       (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS      (1)
#define MICROPY_PY_ALL_INPLACE_SPECIAL_METHODS (1)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS  (1)
#define MICROPY_PY_BUILTINS_COMPILE         (1)
#define MICROPY_PY_BUILTINS_ENUMERATE       (1)
#define MICROPY_PY_BUILTINS_EXECFILE        (1)
#define MICROPY_PY_BUILTINS_FILTER          (1)
#define MICROPY_PY_BUILTINS_REVERSED        (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED  (1)
#define MICROPY_PY_BUILTINS_INPUT           (1)
#define MICROPY_PY_BUILTINS_MIN_MAX         (1)
#define MICROPY_PY_BUILTINS_POW3            (1)
#define MICROPY_PY_BUILTINS_FLOAT           (1)
#define MICROPY_PY_BUILTINS_HELP            (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT       xt804_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES    (1)
#define MICROPY_PY___FILE__                 (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO     (1)
#define MICROPY_PY_ARRAY                    (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN       (1)
#define MICROPY_PY_ATTRTUPLE                (1)
#define MICROPY_PY_COLLECTIONS              (1)
#define MICROPY_PY_COLLECTIONS_DEQUE        (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT  (1)
#define MICROPY_PY_MATH                     (1)
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS   (1)
#define MICROPY_PY_MATH_ISCLOSE             (1)
#define MICROPY_PY_CMATH                    (1)
#define MICROPY_PY_GC                       (1)
#define MICROPY_PY_IO                       (1)
#define MICROPY_PY_IO_IOBASE                (1)
#define MICROPY_PY_IO_FILEIO                (1)
#define MICROPY_PY_IO_BYTESIO               (1)
#define MICROPY_PY_IO_BUFFEREDWRITER        (1)
#define MICROPY_PY_STRUCT                   (1)
#define MICROPY_PY_SYS                      (1)
#define MICROPY_PY_SYS_MAXSIZE              (1)
#define MICROPY_PY_SYS_MODULES              (1)
#define MICROPY_PY_SYS_EXIT                 (1)
#define MICROPY_PY_SYS_STDFILES             (1)
#define MICROPY_PY_SYS_STDIO_BUFFER         (1)
#define MICROPY_PY_UERRNO                   (1)
#define MICROPY_PY_URANDOM                  (1)
#define MICROPY_PY_USELECT                  (1)
#define MICROPY_PY_UTIME_MP_HAL             (1)
#define MICROPY_PY_OS_DUPTERM               (1)
#define MICROPY_PY_UWEBSOCKET               (0)

// config machine modules
#define MICROPY_PY_MACHINE                  (1)
#define MICROPY_PY_MACHINE_BITSTREAM        (1)


///#define MICROPY_PY_THREAD                   (1)
///#define MICROPY_PY_THREAD_GIL               (1)
///#define MICROPY_PY_THREAD_GIL_VM_DIVISOR    (32)


// fatfs configuration
#define MICROPY_FATFS_ENABLE_LFN            (1)
#define MICROPY_FATFS_RPATH                 (2)
#define MICROPY_FATFS_MAX_SS                (4096)
#define MICROPY_FATFS_LFN_CODE_PAGE         437 /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */
#define mp_type_fileio                      mp_type_vfs_fat_fileio
#define mp_type_textio                      mp_type_vfs_fat_textio

#ifndef MICROPY_HW_ENABLE_INTERNAL_FLASH_STORAGE
#define MICROPY_HW_ENABLE_INTERNAL_FLASH_STORAGE (1)
#endif

#if 0
// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
     { MP_OBJ_NEW_QSTR(MP_QSTR_input), (mp_obj_t)&mp_builtin_input_obj }, \
     { MP_OBJ_NEW_QSTR(MP_QSTR_open), (mp_obj_t)&mp_builtin_open_obj },

// extra built in modules to add to the list of known ones
extern const struct _mp_obj_module_t utime_module;
extern const struct _mp_obj_module_t uos_module;
extern const struct _mp_obj_module_t mp_module_usocket;
extern const struct _mp_obj_module_t mp_module_machine;
extern const struct _mp_obj_module_t mp_module_network;
extern const struct _mp_obj_module_t mp_module_onewire;


#define MICROPY_PORT_BUILTIN_MODULES \
     { MP_OBJ_NEW_QSTR(MP_QSTR_utime), (mp_obj_t)&utime_module }, \
     { MP_OBJ_NEW_QSTR(MP_QSTR_uos), (mp_obj_t)&uos_module }, \
     { MP_OBJ_NEW_QSTR(MP_QSTR_usocket), (mp_obj_t)&mp_module_usocket }, \
     { MP_OBJ_NEW_QSTR(MP_QSTR_machine), (mp_obj_t)&mp_module_machine }, \
     { MP_OBJ_NEW_QSTR(MP_QSTR_network), (mp_obj_t)&mp_module_network }, \
     { MP_OBJ_NEW_QSTR(MP_QSTR_onewire), (mp_obj_t)&mp_module_onewire }, \

#endif


#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },

#if 0
extern const struct _mp_obj_module_t mp_module_machine;
extern const struct _mp_obj_module_t mp_module_os;
extern const struct _mp_obj_module_t mp_module_time;
#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_utime), MP_ROM_PTR(&mp_module_time) }, \
    { MP_ROM_QSTR(MP_QSTR_umachine), MP_ROM_PTR(&mp_module_machine) }, \
    { MP_ROM_QSTR(MP_QSTR_uos), MP_ROM_PTR(&mp_module_os) }, \

#endif

extern const struct _mp_obj_module_t utime_module;
extern const struct _mp_obj_module_t uos_module;;
extern const struct _mp_obj_module_t mp_module_machine;
extern const struct _mp_obj_module_t xt804_module;
#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_utime), (mp_obj_t)&utime_module }, \
    { MP_ROM_QSTR(MP_QSTR_uos), MP_ROM_PTR(&uos_module) }, \
    { MP_ROM_QSTR(MP_QSTR_xt804), MP_ROM_PTR(&xt804_module) }, \
    { MP_ROM_QSTR(MP_QSTR_umachine), MP_ROM_PTR(&mp_module_machine) }, \
    


// use vfs's functions for import stat and builtin open
#if MICROPY_VFS
#   define mp_import_stat mp_vfs_import_stat
#   define mp_builtin_open mp_vfs_open
#   define mp_builtin_open_obj mp_vfs_open_obj
#endif

// ----------------
#define MP_SSIZE_MAX                        (0x7FFFFFFF)
#define MP_NEED_LOG2                        (1)

#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
    } while (0);

#define MICROPY_THREAD_YIELD()


#define MICROPY_MPHALPORT_H                 "mphalport.h"


//----------------------------------------------------------
#define MP_STATE_PORT MP_STATE_VM
#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8]; \
    mp_obj_t machine_pin_irq_handler[44]; \
    \
    /*mp_obj_t pyb_hid_report_desc; */\
    \
    /*mp_obj_t pyb_config_main; */\
    \
    /*mp_obj_t pyb_switch_callback; */\
    \
    /*mp_obj_t pin_class_mapper; */\
    /*mp_obj_t pin_class_map_dict; */\
    \
    /*mp_obj_t pyb_extint_callback[PYB_EXTI_NUM_VECTORS]; */\
    \
    /* pointers to all Timer objects (if they have been created) */ \
    /*struct _pyb_timer_obj_t *pyb_timer_obj_all[MICROPY_HW_MAX_TIMER]; */\
    \
    /* stdio is repeated on this UART object if it's not null */ \
    /*struct _pyb_uart_obj_t *pyb_stdio_uart; */\
    \
    /* pointers to all UART objects (if they have been created) */ \
    /*struct _pyb_uart_obj_t *pyb_uart_obj_all[MICROPY_HW_MAX_UART + MICROPY_HW_MAX_LPUART]; */\
    \
    /* pointers to all CAN objects (if they have been created) */ \
    /*struct _pyb_can_obj_t *pyb_can_obj_all[MICROPY_HW_MAX_CAN]; */\
    \
    /* pointers to all I2S objects (if they have been created) */ \
    /*struct _machine_i2s_obj_t *machine_i2s_obj[MICROPY_HW_MAX_I2S]; */\
    \
    /* USB_VCP IRQ callbacks (if they have been set) */ \
    /*mp_obj_t pyb_usb_vcp_irq[MICROPY_HW_USB_CDC_NUM]; */\
    \
    /* list of registered NICs */ \
    /*mp_obj_list_t mod_network_nic_list; */\
    \
    /* root pointers for sub-systems */ \
    /*MICROPY_PORT_ROOT_POINTER_MBEDTLS */\
    /*MICROPY_PORT_ROOT_POINTER_BLUETOOTH_NIMBLE */\
    /*MICROPY_PORT_ROOT_POINTER_BLUETOOTH_BTSTACK */\
    \
    /* root pointers defined by a board */ \
    /*    MICROPY_BOARD_ROOT_POINTERS */\

