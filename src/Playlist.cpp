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
    strcpy(entry.title, RETURN_ENTRY_TITLE);
    entries.emplace(entries.begin(), entry);
}

PlaylistEntry *Playlist::getCurrentEntry() {
    return &entries.at(selectedPosition);
}

std::vector<PlaylistEntry *> Playlist::getWindow() {
    return window;
}

bool Playlist::isAtLastEntry() const {
    return selectedPosition == getSize() - 1;
}

char *Playlist::getSearchableText(const int index) {
    return entries[index].title;
}

void Playlist::sort() {
    std::sort(entries.begin(), entries.end(), [this](const PlaylistEntry &a, const PlaylistEntry &b) {
        if (strcmp(a.title, RETURN_ENTRY_TITLE) == 0) return true;
        if (strcmp(b.title, RETURN_ENTRY_TITLE) == 0) return false;
        return strcasecmp(a.title, b.title) < 0;
    });
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

bool Playlist::isRegularFile(const FILINFO *fileInfo) {
    return fileInfo->fattrib == 32 && fileInfo->fname[0] != 46;
}

void Playlist::markCurrentEntryAsUnplayable() {
    getCurrentEntry()->unplayable = true;
}
