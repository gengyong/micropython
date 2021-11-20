#include <time.h>

#include "xt804_mphal.h"

 #undef ntohl
uint32_t ntohl(uint32_t netlong) {
    return MP_BE32TOH(netlong);
}

#undef htonl
uint32_t htonl(uint32_t netlong) {
    return MP_HTOBE32(netlong);
}

time_t time(time_t * t) {
    return mp_hal_ticks_ms() / 1000;
}

time_t mktime(struct tm * tm_p) {
    return 0;
}

/* rand_r - a reentrant pseudo-random integer on 0..32767 */
int rand_r(unsigned int *nextp) {
    *nextp = *nextp * 1103515245 + 12345;
    return (unsigned int)(*nextp / 65536) % 32768;
}