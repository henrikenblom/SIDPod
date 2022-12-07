#include <malloc.h>
#include <pico/printf.h>
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
    if (!verify_filesystem(fs)) {
        BYTE work[FLASH_SECTOR_SIZE];
        f_mkfs("", 0, work, FLASH_SECTOR_SIZE);
        f_setlabel(FS_LABEL);
        f_mount(fs, "", FA_READ);
    }
    free(fs);
    f_unmount("");
}

bool verify_filesystem(FATFS *fs) {
    DWORD fre_clust, tot_sect;
    if (f_mount(fs, "", FA_READ) != FR_OK) return false;
    if (f_getfree("", &fre_clust, &fs) != FR_OK) return false;
    if ((fs->n_fatent - 2) * fs->csize != SECTOR_COUNT) return false;
    return true;
}