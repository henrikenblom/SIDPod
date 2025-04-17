#include <cstring>
#include <vector>
#include <cstdio>
#include "Playlist.h"

#include <algorithm>
#include <f_util.h>

#include "platform_config.h"

void Playlist::refresh() {
    entries.clear();
    char const *p_dir;
    DIR dj = {};
    FILINFO fno = {};
    FRESULT fr;
    p_dir = name;
    fr = f_findfirst(&dj, &fno, p_dir, "*.sid");
    while (fr == FR_OK && fno.fname[0]) {
        if (isRegularFile(&fno)) {
            tryToAddAsPsid(&fno);
        }
        fr = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
    std::sort(entries.begin(), entries.end(), [](const PlaylistEntry &a, const PlaylistEntry &b) -> bool {
        return strcmp(a.title, b.title) < 0;
    });
    addReturnEntry();
    resetAccessors();
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

size_t Playlist::getSize() {
    return entries.size();
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
    ready = true;
    if (getSize() > 0) {
        updateWindow();
    }
}

void Playlist::getFullPathForSelectedEntry(TCHAR *fullPath) {
    snprintf(fullPath, FF_LFN_BUF + 1, "%s/%s", name, entries.at(selectedPosition).fileName);
}

//TODO: Use the load routine from C64.cpp to validate the playability of the file
void Playlist::tryToAddAsPsid(FILINFO *fileInfo) {
    FIL pFile;
    BYTE header[SID_MINIMAL_HEADER_SIZE];
    UINT bytesRead;
    TCHAR fullPath[FF_LFN_BUF + 1];
    snprintf(fullPath, FF_LFN_BUF + 1, "%s/%s", name, fileInfo->fname);
    f_open(&pFile, fullPath, FA_READ);
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

bool Playlist::isRegularFile(FILINFO *fileInfo) {
    return fileInfo->fattrib == 32 && fileInfo->fname[0] != 46;
}

void Playlist::markCurrentEntryAsUnplayable() {
    getCurrentEntry()->unplayable = true;
}
