#ifndef _ALLOCA_H
#define _ALLOCA_H

#undef alloca

#ifdef __GNUC__
#define alloca(size) __builtin_alloca(size)
#else
void * _EXFUN(alloca,(size_t));
#endif


#endif

