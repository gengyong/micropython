
#ifndef MPY_XT804_HAL_COMMON_H_
#define MPY_XT804_HAL_COMMON_H_

extern int8_t g_colorful_print;
void hal_print(const char * label, const char * term, const char * fmt, ...);


#if (defined(NDEBUG) && NDEBUG)

#define TLOG(args...)
#define TDEBUG(args...)
#define TWARN(args...)  hal_print(g_colorful_print ? "\r\n\x1b[30;43m[WARN]\x1B[0m\x1b[33m " : "[WARN] ", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);
#define TERROR(args...) hal_print(g_colorful_print ? "\r\n\x1b[97;41m[ERROR]\x1B[0m\x1b[31m " : "[ERROR] ", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);
#define TPANIC(args...) hal_print(g_colorful_print ? "\r\n\x1b[5m\x1b[30;101m[PANIC] " : "[PANIC]", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);

#else

#define TLOG(args...)   hal_print(g_colorful_print ? "\r\n\x1b[97;42m[LOG]\x1B[0m\x1b[32m " : "[LOG] ", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);
#define TDEBUG(args...) hal_print(g_colorful_print ? "\r\n\x1b[30;41m[DEBUG]\x1B[0m\x1b[32m " : "[DEBUG] ", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);
#define TWARN(args...)  hal_print(g_colorful_print ? "\r\n\x1b[30;43m[WARN]\x1B[0m\x1b[33m " : "[WARN] ", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);
#define TERROR(args...) hal_print(g_colorful_print ? "\r\n\x1b[97;41m[ERROR]\x1B[0m\x1b[31m " : "[ERROR] ", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);
#define TPANIC(args...) hal_print(g_colorful_print ? "\r\n\x1b[5m\x1b[30;101m[PANIC] " : "[PANIC] ", g_colorful_print ? "\x1b[0m\r\n" : "\r\n", args);

#endif


#endif