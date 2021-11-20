

#ifndef __FIX_STD_HEADER_H
#define __FIX_STD_HEADER_H

#include <time.h>
#include <stdint.h>
#include <string.h>

typedef int32_t __int32_t;
typedef uint32_t __uint32_t;
typedef int16_t __int16_t;
typedef uint16_t __uint16_t;
typedef int8_t __int8_t;
typedef uint8_t __uint8_t;

// missed defination in minilibc
// There's no unistd.h, but this is the equivalent
#ifndef F_OK
#define F_OK 0
#endif

#ifndef W_OK
#define W_OK 2
#endif

#ifndef R_OK
#define R_OK 4
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif


#endif