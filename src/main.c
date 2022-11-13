#include <malloc.h>
#include "main.h"
#include <hardware/flash.h>
#include "ff.h"
#include "sid.h"
#include "device/usbd.h"
#include "msc_control.h"

FATFS *fs;
const uint32_t PSID_ID = 0x50534944;

void tud_mount_cb(void) {
    f_unmount("");
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    set_msc_ready_to_attach();
    f_mount(fs, "", FA_READ | FA_WRITE);
    index_psid_files();
}

void index_psid_files() {
    DIR *dp;
    FILINFO fno;
    FRESULT fr;
    dp = malloc(sizeof(DIR));
    f_opendir(dp, "");
    fr = f_findfirst(dp, &fno, "", "*.sid");
    while (fr == FR_OK && fno.fname[0]) {
        if (is_PSID_file(fno)) {
            printf("%s", fno.fname);
        }
        fr = f_findnext(dp, &fno);
    }
    f_closedir(dp);
}

bool is_PSID_file(FILINFO filinfo) {
    FIL fil;
    BYTE header[4];
    UINT bytes_read;
    bool retval = false;
    f_open(&fil, filinfo.fname, FA_READ);
    f_read(&fil, &header, 4, &bytes_read);
    if (bytes_read == 4) {
        uint32_t magic = header[3] | (header[2] << 8) | (header[1] << 16) | (header[0] << 24);
        retval = magic == PSID_ID;
    }
    f_close(&fil);
    return retval;
}

void filesystem_init() {
    BYTE work[FLASH_SECTOR_SIZE];
    fs = malloc(sizeof(FATFS));
    if (f_mount(fs, "", FA_READ | FA_WRITE) != FR_OK) {
        f_mkfs("", 0, work, sizeof(work));
        f_setlabel(FS_LABEL);
        f_mount(fs, "", FA_READ | FA_WRITE);
    }
    f_unmount("");
}

int main() {
    filesystem_init();
    f_mount(fs, "", FA_READ | FA_WRITE);
    index_psid_files();
    tud_init(BOARD_TUD_RHPORT);

    while (true) {
        tud_task();
    }
}