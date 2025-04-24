//
// Created by Henrik Enblom on 2025-04-12.
//

#include "Catalog.h"
#include <cstring>
#include <sd_card.h>
#include <vector>
#include <string>

#include "platform_config.h"
#include "Playlist.h"

std::vector<std::string> entries;
Playlist *currentPlaylist = nullptr;
int topItem = 0;
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
        sd_card_t *sd_card_p = sd_get_by_drive_prefix("0:");
        FATFS *fs_p = &sd_card_p->state.fatfs;
        f_mount(fs_p, "0:", 1);
        sd_card_p->state.mounted = true;
        dp = new DIR;
        f_opendir(dp, "");
        int c = 0;
        entries.clear();
        while (entries.size() < CATALOG_WINDOW_SIZE) {
            fr = f_readdir(dp, &fno);
            if (fr != FR_OK || fno.fname[0] == 0) break;
            if (isValidDirectory(&fno)) {
                if (c++ >= topItem) {
                    entries.emplace_back(fno.fname);
                    if (selected.empty()) {
                        selected = fno.fname;
                    }
                }
            }
        }

        f_closedir(dp);
        delete dp;
        refreshing = false;
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

std::vector<std::string> *Catalog::getEntries() {
    return &entries;
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
    if (topItem <= entries.size() - CATALOG_WINDOW_SIZE + 2) {
        topItem++;
    }
    refresh();
}

void Catalog::slideUp() {
    if (topItem > 0) {
        topItem--;
    }
    refresh();
}

void Catalog::selectNext() {
    for (int i = 0; i < entries.size(); i++) {
        if (entries[i] == selected && i != entries.size() - 1) {
            selected = entries[i + 1];
            break;
        }
    }
    slideDown();
}

void Catalog::selectPrevious() {
    for (int i = 0; i < entries.size(); i++) {
        if (entries[i] == selected && i > 0) {
            selected = entries[i - 1];
            break;
        }
    }
    slideUp();
}

void Catalog::goHome() {
    if (!refreshing) {
        topItem = 0;
        selected.clear();
        refresh();
    }
}

Playlist *Catalog::getCurrentPlaylist() {
    return currentPlaylist;
}
