#include "ff.h"
#include "hardware/rtc.h"

#ifndef MP_WEAK
#define MP_WEAK __attribute__((weak))
#endif

MP_WEAK DWORD get_fattime(void) {
    datetime_t t;
    rtc_get_datetime(&t);
    return ((t.year - 1980) << 25) | ((t.month) << 21) | ((t.day) << 16) | ((t.hour) << 11) | ((t.min) << 5) |
           (t.sec / 2);
}
