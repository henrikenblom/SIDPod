//
// Created by Henrik Enblom on 2025-04-12.
//

#include "Catalog.h"

#ifdef USE_SDCARD
#include "sd_card.h"
#endif
#include <algorithm>
#include <cstring>
#include <vector>
#include <string>

#include "platform_config.h"
#include "Playlist.h"

Catalog *catalog = new Catalog();

int Catalog::initRefresh() {
    if (state != REFRESHING) {
        state = REFRESHING;
        dp = new DIR;
        entries.clear();
        candidateIndex = 0;
        candidateCount = 0;
        f_opendir(dp, "");
        FILINFO fno;
        while (f_readdir(dp, &fno) == FR_OK && fno.fname[0] != 0) {
            if (isValidDirectory(&fno)) {
                candidateCount++;
            }
        }
        f_rewinddir(dp);
        return candidateCount;
    }
    return 0;
}

bool Catalog::advanceRefresh() {
    if (state == REFRESHING) {
        FILINFO fno;
        for (int i = 0; i < std::max(1, candidateCount / 10); i++) {
            if (f_readdir(dp, &fno) == FR_OK
                && fno.fname[0] != 0
                && entries.size() < MAX_LIST_ENTRIES) {
                candidateIndex++;
                if (isValidDirectory(&fno)) {
                    CatalogEntry entry = {};
                    strcpy(entry.name, fno.fname);
                    entry.selected = entries.empty();
                    entries.emplace_back(entry);
                }
            } else {
                f_closedir(dp);
                delete dp;
                state = READY;
                resetAccessors();
                break;
            }
        }
        return true;
    }
    return false;
}

void Catalog::setSelectedPlaying() {
    for (auto &entry: entries) {
        entry.playing = false;
    }
    if (!filteredEntries.empty() && selectedPosition < static_cast<int>(getFilteredSize())) {
        filteredEntries[selectedPosition]->playing = true;
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
    currentPlaylist = new Playlist(filteredEntries.at(selectedPosition)->name);
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

void Catalog::markAsFound(const int index, const char position) {
    entries[index].foundStart = position + 1;
}

void Catalog::unmarkAsFound(const int index) {
    entries[index].foundStart = 0;
}

void Catalog::sort() {
    std::sort(entries.begin(), entries.end(), [this](const CatalogEntry &a, const CatalogEntry &b) {
        return strcasecmp(a.name, b.name) < 0;
    });
}
