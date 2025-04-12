//
// Created by Henrik Enblom on 2025-04-12.
//

#include "Catalog.h"
#include <cstring>
#include <vector>
#include <cstdio>

#include "platform_config.h"
#include "Playlist.h"

std::vector<Playlist> catalogEntries(50);

void Catalog::refresh() {
    DIR *dp;
    FILINFO fno;
    FRESULT fr;
    dp = new DIR;
    catalogEntries.clear();
    f_opendir(dp, "");
    while (true) {
        fr = f_readdir(dp, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;
        if (isValidDirectory(&fno) && containsSIDs(fno.fname)) {
            printf("Found directory: %s\n", fno.fname);
            printf("\n");
        }
    }
    f_closedir(dp);
    delete dp;
}

bool Catalog::containsSIDs(TCHAR *path) {
    DIR *dp = new DIR;
    FILINFO fno;
    FRESULT fr;
    FIL pFile;
    TCHAR fullPath[FF_LFN_BUF + 1];
    f_opendir(dp, path);
    BYTE header[SID_MINIMAL_HEADER_SIZE];
    bool isValid = false;
    while (true) {
        fr = f_readdir(dp, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;
        if (!(fno.fattrib & AM_HID) && !(fno.fattrib & AM_DIR) && fno.fname[0] != 46) {
            UINT bytesRead;
            snprintf(fullPath, sizeof path + sizeof fno.fname + 1, "%s/%s", path, fno.fname);
            f_open(&pFile, fullPath, FA_READ);
            f_read(&pFile, &header, SID_MINIMAL_HEADER_SIZE, &bytesRead);
            if (bytesRead == SID_MINIMAL_HEADER_SIZE) {
                uint32_t magic = header[3] | header[2] << 0x08 | header[1] << 0x10 | header[0] << 0x18;
                if (magic == PSID_ID || magic == RSID_ID) {
                    isValid = true;
                    break;
                }
            }
        }
    }
    f_close(&pFile);
    f_closedir(dp);
    delete dp;
    return isValid;
}
