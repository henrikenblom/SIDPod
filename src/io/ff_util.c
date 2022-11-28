#include <malloc.h>
#include "ff_util.h"
#include "hardware/rtc.h"
#include "../platform_config.h"

#ifndef MP_WEAK
#define MP_WEAK __attribute__((weak))
#endif

MP_WEAK DWORD get_fattime(void) {
    datetime_t t;
    rtc_get_datetime(&t);
    return ((t.year - 1980) << 25) | ((t.month) << 21) | ((t.day) << 16) | ((t.hour) << 11) | ((t.min) << 5) |
           (t.sec / 2);
}

void filesystem_init() {
    FATFS *fs;
    fs = malloc(sizeof(FATFS));
    if (f_mount(fs, "", FA_READ) != FR_OK) {
        BYTE work[FLASH_SECTOR_SIZE];
        f_mkfs("", 0, work, FLASH_SECTOR_SIZE);
        f_setlabel(FS_LABEL);
        f_mount(fs, "", FA_READ);
    }
    free(fs);
    f_unmount("");
}