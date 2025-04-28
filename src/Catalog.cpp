//
// Created by Henrik Enblom on 2025-04-12.
//

#include "Catalog.h"

#include <algorithm>
#include <sd_card.h>
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
