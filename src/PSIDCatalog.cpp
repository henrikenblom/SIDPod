#include <cstring>
#include <vector>
#include "PSIDCatalog.h"
#include "sid.h"
#include "PSIDCatalogEntry.h"

#define WINDOW_SIZE 3;

FATFS *fs = new FATFS;
uint32_t PSID_ID = 0x50534944;
uint8_t PSID_HEADER_SIZE = 0x88;
std::vector<PSIDCatalogEntry> catalog;
std::vector<PSIDCatalogEntry> window;
std::vector<PSIDCatalogEntry>::iterator catalogIterator;
uint8_t windowPosition = 0;
uint8_t selectedPosition = 0;
uint8_t windowSize = WINDOW_SIZE;

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
    resetAccessors();
}

PSIDCatalogEntry PSIDCatalog::getNextPsidFile() {
    PSIDCatalogEntry psidFile = *catalogIterator++;
    if (catalogIterator == catalog.end()) {
        catalogIterator = catalog.begin();
    }
    return psidFile;
}

size_t PSIDCatalog::getSize() {
    return catalog.size();
}

std::vector<PSIDCatalogEntry> PSIDCatalog::getWindow() {
    return window;
}

void PSIDCatalog::selectNext() {
    if (selectedPosition < getSize() - 1) {
        selectedPosition++;
        slideDown();
        updateWindow();
    }
}

void PSIDCatalog::selectPrevious() {
    if (selectedPosition > 0) {
        selectedPosition--;
        slideUp();
        updateWindow();
    }
}

void PSIDCatalog::resetAccessors() {
    catalogIterator = catalog.begin();
    selectedPosition = 0;
    windowPosition = 0;
    if (getSize() > windowSize) {
        updateWindow();
    } else {
        window = catalog;
    }
}

void PSIDCatalog::tryToAddAsPsid(FILINFO fileInfo) {
    FIL pFile;
    BYTE header[PSID_HEADER_SIZE];
    UINT bytesRead;
    f_open(&pFile, fileInfo.fname, FA_READ);
    f_read(&pFile, &header, PSID_HEADER_SIZE, &bytesRead);
    if (bytesRead == PSID_HEADER_SIZE) {
        uint32_t magic = header[3] | (header[2] << 0x08) | (header[1] << 0x10) | (header[0] << 0x18);
        if (magic == PSID_ID) {
            PSIDCatalogEntry psidFile = PSIDCatalogEntry(fileInfo);
            auto *pHeader = (unsigned char *) header;
            strcpy(psidFile.title, (const char *) &pHeader[0x16]);
            strcpy(psidFile.author, (const char *) &pHeader[0x36]);
            strcpy(psidFile.released, (const char *) &pHeader[0x56]);
            catalog.push_back(psidFile);
        }
    }
    f_close(&pFile);
}


void PSIDCatalog::slideDown() {
    if (windowSize + windowPosition < getSize()) {
        windowPosition++;
    }
}

void PSIDCatalog::slideUp() {
    if (windowPosition > 0) {
        windowPosition--;
    }
}

void PSIDCatalog::updateWindow() {
    window.clear();
    for (int i = 0; i < windowSize; i++) {
        auto entry = catalog.at(windowPosition + i);
        entry.selected = windowPosition + i == selectedPosition;
        window.push_back(entry);
    }
}
