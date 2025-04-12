//
// Created by Henrik Enblom on 2025-04-12.
//

#include "Catalog.h"
#include <cstring>
#include <vector>
#include <cstdio>
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
        if (isValidDirectory(&fno)) {
            printf("Found directory: %s\n", fno.altname);
            printf("fname: %s\n", fno.fname);
            printf("Attrib: %d\n", fno.fattrib);
            printf("\n");
        }
    }
    f_closedir(dp);
    delete dp;
}
