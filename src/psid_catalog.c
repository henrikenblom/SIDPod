#include <malloc.h>
#include <pico/printf.h>
#include "psid_catalog.h"
#include "ff.h"

FATFS *fs;

void index_psid_files() {
    DIR *dp;
    FILINFO fno;
    FRESULT fr;
    dp = malloc(sizeof(DIR));
    if (!fs) {
        fs = malloc(sizeof(FATFS));
    }
    f_mount(fs, "", FA_READ | FA_WRITE);
    f_opendir(dp, "");
    fr = f_findfirst(dp, &fno, "", "*.sid");
    while (fr == FR_OK && fno.fname[0]) {
        if (is_PSID_file(fno)) {
            printf("%s", fno.fname);
        }
        fr = f_findnext(dp, &fno);
    }
    f_closedir(dp);
    free(dp);
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