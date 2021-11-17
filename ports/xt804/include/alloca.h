#ifndef _ALLOCA_H
#define _ALLOCA_H

#undef alloca

#ifdef __GNUC__
#define alloca(size) __builtin_alloca(size)
#else
void * _EXFUN(alloca,(size_t));
#endif


//  missed defination in minilibc
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


// typedef int32_t time_t;
// //typedef UINT32 clock_t;
// typedef int32_t suseconds_t;

// typedef struct xt804_timeval timeval;

// struct xt804_timeval
// {
//     time_t        tv_sec;                  /* seconds */
//     suseconds_t   tv_usec;                 /* microseconds */
// };

#endif

