//
// Created by Henrik Enblom on 2025-04-12.
//

#include "Catalog.h"

#include <algorithm>
#ifdef USE_SDCARD
#include "sd_card.h"
#endif
#include <cstring>
#include <vector>
#include <string>

#include "platform_config.h"
#include "Playlist.h"

std::vector<std::string> entries;
std::vector<std::string *> window;
Playlist *currentPlaylist = nullptr;
uint8_t selectedPosition = 0;
uint8_t windowPosition = 0;
uint8_t windowSize = CATALOG_WINDOW_SIZE;
std::string selected;
std::string playing;
bool playlistOpen = false;
bool refreshing = false;

void Catalog::refresh() {
    if (!refreshing) {
        refreshing = true;
        DIR *dp;
        FILINFO fno;
        FRESULT fr;
        dp = new DIR;
        f_opendir(dp, "");
        entries.clear();
        while (entries.size() < MAX_LIST_ENTRIES) {
            fr = f_readdir(dp, &fno);
            if (fr != FR_OK || fno.fname[0] == 0) break;
            if (isValidDirectory(&fno)) {
                entries.emplace_back(fno.fname);
            }
        }

        f_closedir(dp);
        delete dp;
        std::sort(entries.begin(), entries.end(), [](const std::string &a, const std::string &b) -> bool {
            return strcmp(a.c_str(), b.c_str()) < 0;
        });
        refreshing = false;
        resetAccessors();
    }
}

std::string Catalog::getPlaying() {
    return playing;
}

void Catalog::setPlaying(const std::string &_playing) {
    playing = _playing;
}

std::string Catalog::getSelected() {
    return selected;
}

std::vector<std::string *> Catalog::getWindow() {
    return window;
}

bool Catalog::playlistIsOpen() {
    return playlistOpen;
}

void Catalog::closeSelected() {
    if (currentPlaylist != nullptr) {
        delete currentPlaylist;
        currentPlaylist = nullptr;
    }
    playlistOpen = false;
}

void Catalog::openSelected() {
    delete currentPlaylist;
    currentPlaylist = new Playlist(selected.c_str());
    playlistOpen = true;
}

void Catalog::slideDown() {
    if (windowSize + windowPosition < getSize()) {
        windowPosition++;
    }
}

void Catalog::slideUp() {
    if (windowPosition > 0) {
        windowPosition--;
    }
}

void Catalog::selectNext() {
    if (selectedPosition < getSize() - 1) {
        selectedPosition++;
        slideDown();
        updateWindow();
    }
}

void Catalog::selectPrevious() {
    if (selectedPosition > 0) {
        selectedPosition--;
        slideUp();
        updateWindow();
    }
}

void Catalog::selectLast() {
    selectedPosition = getSize() - 1;
    windowPosition = getSize() - windowSize;
    updateWindow();
}

size_t Catalog::getSize() {
    return entries.size();
}

Playlist *Catalog::getCurrentPlaylist() {
    return currentPlaylist;
}

bool Catalog::isPSID(char *fullPath) {
    bool result = false;
    FIL pFile;
    BYTE header[SID_MINIMAL_HEADER_SIZE];
    UINT bytesRead;
    f_open(&pFile, fullPath, FA_READ);
    f_read(&pFile, &header, SID_MINIMAL_HEADER_SIZE, &bytesRead);
    if (bytesRead == SID_MINIMAL_HEADER_SIZE) {
        uint32_t magic = header[3] | header[2] << 0x08 | header[1] << 0x10 | header[0] << 0x18;
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

void Catalog::updateWindow() {
    if (getSize()) {
        window.clear();
        for (int i = 0; i < std::min(windowSize, static_cast<uint8_t>(getSize())); i++) {
            auto entry = &entries.at(windowPosition + i);
            window.push_back(entry);
        }
        selected = entries.at(selectedPosition);
    }
}

void Catalog::resetAccessors() {
    selectedPosition = 0;
    windowPosition = 0;
    if (getSize() > 0) {
        updateWindow();
    }
}
