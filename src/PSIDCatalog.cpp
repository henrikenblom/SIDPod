#include <cstring>
#include <vector>
#include <cstdio>
#include "PSIDCatalog.h"

#include <algorithm>
#include <string>

#include "platform_config.h"

FATFS *fs = new FATFS;
std::vector<CatalogEntry> catalog;
std::vector<CatalogEntry *> window;
uint8_t windowPosition = 0;
uint8_t selectedPosition = 0;
uint8_t windowSize = CATALOG_WINDOW_SIZE;

void PSIDCatalog::refresh() {
    DIR *dp;
    FILINFO fno;
    FRESULT fr;
    dp = new DIR;
    catalog.clear();
    f_mount(fs, "", FA_READ);
    f_opendir(dp, "");
    while (true) {
        fr = f_readdir(dp, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;
        if (isRegularFile(&fno)) {
            tryToAddAsPsid(&fno);
        }
    }
    f_closedir(dp);
    delete dp;
    std::sort(catalog.begin(), catalog.end(), [](const CatalogEntry &a, const CatalogEntry &b) -> bool {
        return strcmp(a.title, b.title) < 0;
    });
    resetAccessors();
}

CatalogEntry *PSIDCatalog::getCurrentEntry() {
    return &catalog.at(selectedPosition);
}

size_t PSIDCatalog::getSize() {
    return catalog.size();
}

std::vector<CatalogEntry *> PSIDCatalog::getWindow() {
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
    selectedPosition = 0;
    windowPosition = 0;
    if (getSize() > 0) {
        updateWindow();
    }
}

//TODO: Use the load routine from C64.cpp to validate the playability of the file
void PSIDCatalog::tryToAddAsPsid(FILINFO *fileInfo) {
    FIL pFile;
    BYTE header[SID_MINIMAL_HEADER_SIZE];
    UINT bytesRead;
    f_open(&pFile, fileInfo->altname, FA_READ);
    f_read(&pFile, &header, SID_MINIMAL_HEADER_SIZE, &bytesRead);
    if (bytesRead == SID_MINIMAL_HEADER_SIZE) {
        uint32_t magic = header[3] | header[2] << 0x08 | header[1] << 0x10 | header[0] << 0x18;
        bool isRsid = magic == RSID_ID;
        if (magic == PSID_ID || isRsid) {
            auto *pHeader = static_cast<unsigned char *>(header);
            CatalogEntry entry{};
            entry.unplayable = false;
            strcpy(entry.fileName, fileInfo->altname);
            strcpy(entry.title, reinterpret_cast<const char *>(&pHeader[0x16]));
            entry.rsid = isRsid;
            catalog.push_back(entry);
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
    if (getSize()) {
        window.clear();
        for (int i = 0; i < std::min(windowSize, (uint8_t) getSize()); i++) {
            auto entry = &catalog.at(windowPosition + i);
            entry->selected = windowPosition + i == selectedPosition;
            window.push_back(entry);
        }
    }
}

bool PSIDCatalog::isRegularFile(FILINFO *fileInfo) {
    return fileInfo->fattrib == 32 && fileInfo->fname[0] != 46;
}

void PSIDCatalog::markCurrentEntryAsUnplayable() {
    getCurrentEntry()->unplayable = true;
}
