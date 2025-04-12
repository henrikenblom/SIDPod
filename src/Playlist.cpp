#include <cstring>
#include <vector>
#include <cstdio>
#include "Playlist.h"

#include <algorithm>

#include "platform_config.h"

FATFS *fs = new FATFS;
std::vector<PlaylistEntry> catalog(50);
std::vector<PlaylistEntry *> window;
uint8_t windowPosition = 0;
uint8_t selectedPosition = 0;
uint8_t windowSize = CATALOG_WINDOW_SIZE;

void Playlist::refresh() {
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
    std::sort(catalog.begin(), catalog.end(), [](const PlaylistEntry &a, const PlaylistEntry &b) -> bool {
        return strcmp(a.title, b.title) < 0;
    });
    resetAccessors();
}

PlaylistEntry *Playlist::getCurrentEntry() {
    return &catalog.at(selectedPosition);
}

size_t Playlist::getSize() {
    return catalog.size();
}

std::vector<PlaylistEntry *> Playlist::getWindow() {
    return window;
}

void Playlist::selectNext() {
    if (selectedPosition < getSize() - 1) {
        selectedPosition++;
        slideDown();
        updateWindow();
    }
}

void Playlist::selectPrevious() {
    if (selectedPosition > 0) {
        selectedPosition--;
        slideUp();
        updateWindow();
    }
}

void Playlist::resetAccessors() {
    selectedPosition = 0;
    windowPosition = 0;
    if (getSize() > 0) {
        updateWindow();
    }
}

//TODO: Use the load routine from C64.cpp to validate the playability of the file
void Playlist::tryToAddAsPsid(FILINFO *fileInfo) {
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
            PlaylistEntry entry{};
            entry.unplayable = false;
            strcpy(entry.fileName, fileInfo->altname);
            strcpy(entry.title, reinterpret_cast<const char *>(&pHeader[0x16]));
            catalog.push_back(entry);
        }
    }
    f_close(&pFile);
}


void Playlist::slideDown() {
    if (windowSize + windowPosition < getSize()) {
        windowPosition++;
    }
}

void Playlist::slideUp() {
    if (windowPosition > 0) {
        windowPosition--;
    }
}

void Playlist::updateWindow() {
    if (getSize()) {
        window.clear();
        for (int i = 0; i < std::min(windowSize, (uint8_t) getSize()); i++) {
            auto entry = &catalog.at(windowPosition + i);
            entry->selected = windowPosition + i == selectedPosition;
            window.push_back(entry);
        }
    }
}

bool Playlist::isRegularFile(FILINFO *fileInfo) {
    return fileInfo->fattrib == 32 && fileInfo->fname[0] != 46;
}

void Playlist::markCurrentEntryAsUnplayable() {
    getCurrentEntry()->unplayable = true;
}
