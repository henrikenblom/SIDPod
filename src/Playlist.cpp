#include <cstring>
#include <vector>
#include <cstdio>
#include "Playlist.h"

#include <algorithm>
#include "platform_config.h"

void Playlist::refresh() {
    if (state != REFRESHING) {
        state = REFRESHING;
        FILINFO fno;
        auto dp = new DIR;
        entries.clear();
        f_opendir(dp, name);
        FRESULT fr = f_readdir(dp, &fno);
        // TODO: Drive this loop externally, so that we can animate progress in the UI
        while (fr == FR_OK && fno.fname[0] != 0 && entries.size() < MAX_LIST_ENTRIES) {
            if (isRegularFile(&fno)) {
                tryToAddAsPsid(&fno);
            }
            fr = f_readdir(dp, &fno);
        }
        f_closedir(dp);
        delete dp;
        std::sort(entries.begin(), entries.end(), [](const PlaylistEntry &a, const PlaylistEntry &b) -> bool {
            return strcasecmp(a.title, b.title) < 0;
        });
        addReturnEntry();
        resetAccessors();
        state = READY;
    }
}

const char *Playlist::getName() const {
    return name;
}

bool Playlist::isAtReturnEntry() const {
    return selectedPosition == 0;
}

void Playlist::addReturnEntry() {
    PlaylistEntry entry{};
    entry.unplayable = false;
    strcpy(entry.title, "<< Return");
    entries.emplace(entries.begin(), entry);
}

PlaylistEntry *Playlist::getCurrentEntry() {
    return &entries.at(selectedPosition);
}

size_t Playlist::getSize() const {
    return entries.size();
}

std::vector<PlaylistEntry *> Playlist::getWindow() {
    return window;
}

bool Playlist::isAtLastEntry() const {
    return selectedPosition == getSize() - 1;
}

void Playlist::selectNext() {
    if (state == READY) {
        if (selectedPosition < getSize() - 1) {
            selectedPosition++;
            slideDown();
            updateWindow();
        }
    }
}

void Playlist::selectPrevious() {
    if (state == READY) {
        if (selectedPosition > 0) {
            selectedPosition--;
            slideUp();
            updateWindow();
        }
    }
}

void Playlist::selectFirst() {
    if (state == READY) {
        selectedPosition = 1;
        windowPosition = 1;
        updateWindow();
    }
}

void Playlist::selectLast() {
    if (state == READY) {
        selectedPosition = getSize() - 1;
        windowPosition = getSize() - windowSize;
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

void Playlist::enableFind() {
    findEnabled = true;
}

void Playlist::disableFind() {
    findEnabled = false;
    clearSearchString();
    updateWindow();
    if (state == READY) {
        state = READY;
    }
}

bool Playlist::findIsEnabled() const {
    return findEnabled;
}

void Playlist::addToSearchString(char c) {
    if (const size_t len = strlen(searchString); len < sizeof(searchString) - 1) {
        searchString[len] = c;
        searchString[len + 1] = '\0';
    }
    findSearchString();
}

void Playlist::clearSearchString() {
    searchString[0] = '\0';
}

char *Playlist::getSearchString() {
    return searchString;
}

void Playlist::findSearchString() {
    if (searchString[0] == '\0' || state != READY) {
        return;
    }
    for (size_t i = 0; i < entries.size(); ++i) {
        if (strncasecmp(entries[i].title, searchString, strlen(searchString)) == 0) {
            selectedPosition = i;
            windowPosition = std::min(i, entries.size() - windowSize);
            updateWindow();
            break;
        }
    }
}

void Playlist::getFullPathForSelectedEntry(TCHAR *fullPath) {
    snprintf(fullPath, FF_LFN_BUF + 1, "%s/%s", name, entries.at(selectedPosition).fileName);
}

void Playlist::tryToAddAsPsid(FILINFO *fileInfo) {
    FIL pFile;
    BYTE header[PSID_MINIMAL_HEADER_SIZE];
    UINT bytesRead;
    TCHAR fullPath[FF_LFN_BUF + 1];
    snprintf(fullPath, FF_LFN_BUF + 1, "%s/%s", name, fileInfo->fname);
    f_open(&pFile, fullPath, FA_READ);
    f_read(&pFile, &header, PSID_MINIMAL_HEADER_SIZE, &bytesRead);
    if (bytesRead == PSID_MINIMAL_HEADER_SIZE) {
        uint32_t magic = header[3] | header[2] << 0x08 | header[1] << 0x10 | header[0] << 0x18;
        if (magic == PSID_ID || magic == RSID_ID) {
            const auto *pHeader = static_cast<unsigned char *>(header);
            PlaylistEntry entry{};
            entry.unplayable = false;
            strcpy(entry.fileName, fileInfo->altname);
            strcpy(entry.title, reinterpret_cast<const char *>(&pHeader[0x16]));
            strcpy(entry.author, reinterpret_cast<const char *>(&pHeader[0x36]));
            entries.push_back(entry);
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
            auto entry = &entries.at(windowPosition + i);
            entry->selected = windowPosition + i == selectedPosition;
            window.push_back(entry);
        }
    }
}

bool Playlist::isRegularFile(const FILINFO *fileInfo) {
    return fileInfo->fattrib == 32 && fileInfo->fname[0] != 46;
}

void Playlist::markCurrentEntryAsUnplayable() {
    getCurrentEntry()->unplayable = true;
}
