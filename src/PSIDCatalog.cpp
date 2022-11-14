#include <cstring>
#include <vector>
#include "PSIDCatalog.h"
#include "sid.h"
#include "PSIDFile.h"

FATFS *fs = new FATFS;
std::vector<PSIDFile> catalog;
uint32_t PSID_ID = 0x50534944;
uint8_t PSID_HEADER_SIZE = 0x88;
std::vector<PSIDFile>::iterator catalogIterator;

void PSIDCatalog::refreshCatalog() {
    DIR *dp;
    FILINFO fno;
    FRESULT fr;
    dp = new DIR;
    catalog.clear();
    f_mount(fs, "", FA_READ | FA_WRITE);
    f_opendir(dp, "");
    fr = f_findfirst(dp, &fno, "", "*.sid");
    while (fr == FR_OK && fno.fname[0]) {
        tryToAddAsPsid(fno);
        fr = f_findnext(dp, &fno);
    }
    f_closedir(dp);
    delete dp;
    catalogIterator = catalog.begin();
}

void PSIDCatalog::tryToAddAsPsid(FILINFO fileInfo) {
    FIL fil;
    BYTE header[PSID_HEADER_SIZE];
    UINT bytes_read;
    f_open(&fil, fileInfo.fname, FA_READ);
    f_read(&fil, &header, PSID_HEADER_SIZE, &bytes_read);
    if (bytes_read == PSID_HEADER_SIZE) {
        uint32_t magic = header[3] | (header[2] << 0x08) | (header[1] << 0x10) | (header[0] << 0x18);
        if (magic == PSID_ID) {
            PSIDFile psidFile = PSIDFile(fileInfo);
            auto *pHeader = (unsigned char *) header;
            strcpy(psidFile.title, (const char *) &pHeader[0x16]);
            strcpy(psidFile.author, (const char *) &pHeader[0x36]);
            strcpy(psidFile.released, (const char *) &pHeader[0x56]);
            catalog.push_back(psidFile);
        }
    }
    f_close(&fil);
}


PSIDFile PSIDCatalog::getNextPsidFile() {
    PSIDFile psidFile = *catalogIterator++;
    if (catalogIterator == catalog.end()) {
        catalogIterator = catalog.begin();
    }
    return psidFile;
}

size_t PSIDCatalog::getSize() {
    return catalog.size();
}
