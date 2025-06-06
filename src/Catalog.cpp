//
// Created by Henrik Enblom on 2025-04-12.
//

#include "Catalog.h"

#ifdef USE_SDCARD
#include "sd_card.h"
#endif
#include <cstring>
#include <vector>
#include <string>

#include "platform_config.h"
#include "Playlist.h"

Catalog *catalog = new Catalog();
bool refreshing = false;

void Catalog::refresh() {
    if (!refreshing) {
        refreshing = true;
        FILINFO fno;
        const auto dp = new DIR;
        f_opendir(dp, "");
        entries.clear();
        while (entries.size() < MAX_LIST_ENTRIES) {
            if (const FRESULT fr = f_readdir(dp, &fno); fr != FR_OK || fno.fname[0] == 0) break;
            if (isValidDirectory(&fno)) {
                CatalogEntry entry = {};
                strcpy(entry.name, fno.fname);
                entry.selected = entries.empty();
                entries.emplace_back(entry);
            }
        }
        f_closedir(dp);
        delete dp;
        refreshing = false;
        resetAccessors();
    }
}

void Catalog::setSelectedPlaying() {
    for (auto &entry: entries) {
        entry.playing = false;
    }
    if (!entries.empty() && selectedPosition < static_cast<int>(entries.size())) {
        entries[selectedPosition].playing = true;
    }
}

bool Catalog::hasOpenPlaylist() const {
    return currentPlaylist != nullptr;
}

void Catalog::closeSelected() {
    if (currentPlaylist != nullptr) {
        delete currentPlaylist;
        currentPlaylist = nullptr;
    }
}

void Catalog::openSelected() {
    delete currentPlaylist;
    currentPlaylist = new Playlist(entries.at(selectedPosition).name);
}

Playlist *Catalog::getCurrentPlaylist() const {
    return currentPlaylist;
}

bool Catalog::isValidDirectory(FILINFO *fileInfo) {
    return fileInfo->fattrib == AM_DIR && fileInfo->fname[0] != 46 && containsAtLeastOnePSID(fileInfo->fname);
}

bool Catalog::isPSID(const char *fullPath) {
    bool result = false;
    FIL pFile;
    BYTE header[PSID_MINIMAL_HEADER_SIZE];
    UINT bytesRead;
    f_open(&pFile, fullPath, FA_READ);
    f_read(&pFile, &header, PSID_MINIMAL_HEADER_SIZE, &bytesRead);
    if (bytesRead == PSID_MINIMAL_HEADER_SIZE) {
        const uint32_t magic = header[3] | header[2] << 0x08 | header[1] << 0x10 | header[0] << 0x18;
        result = magic == PSID_ID || magic == RSID_ID;
    }
    f_close(&pFile);
    return result;
}

bool Catalog::containsAtLeastOnePSID(char *name) {
    FILINFO fno;
    const auto dp = new DIR;
    bool found = false;
    f_opendir(dp, name);
    FRESULT fr = f_readdir(dp, &fno);
    while (fr == FR_OK && fno.fname[0] != 0) {
        TCHAR fullPath[FF_LFN_BUF + 1];
        snprintf(fullPath, FF_LFN_BUF + 1, "%s/%s", name, fno.fname);
        if (isPSID(fullPath)) {
            found = true;
            break;
        }
        fr = f_readdir(dp, &fno);
    }
    f_closedir(dp);
    delete dp;
    return found;
}

char *Catalog::getSearchableText(const int index) {
    return entries[index].name;
}
