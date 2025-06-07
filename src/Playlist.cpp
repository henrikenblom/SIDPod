#include <cstring>
#include <vector>
#include <cstdio>
#include "Playlist.h"

#include <algorithm>
#include "platform_config.h"

int Playlist::initRefresh() {
    if (state != REFRESHING) {
        state = REFRESHING;
        entries.clear();
        dp = new DIR;
        candidateIndex = 0;
        candidateCount = 0;
        f_opendir(dp, name);
        FILINFO fno;
        while (f_readdir(dp, &fno) == FR_OK && fno.fname[0] != 0) {
            if (isRegularFile(&fno)) {
                candidateCount++;
            }
        }
        f_rewinddir(dp);
        return candidateCount;
    }
    return 0;
}

bool Playlist::advanceRefresh() {
    if (state == REFRESHING) {
        FILINFO fno;
        for (int i = 0; i < std::max(1, candidateCount / 10); i++) {
            if (f_readdir(dp, &fno) == FR_OK
                && fno.fname[0] != 0
                && entries.size() < MAX_LIST_ENTRIES) {
                if (isRegularFile(&fno)) {
                    candidateIndex++;
                    tryToAddAsPsid(&fno);
                }
            } else {
                f_closedir(dp);
                delete dp;
                addReturnEntry();
                resetAccessors();
                state = READY;
                break;
            }
        }
        return true;
    }
    return false;
}

const char *Playlist::getName() const {
    return name;
}

bool Playlist::isAtReturnEntry() const {
    return strcmp(getCurrentEntry()->title, RETURN_ENTRY_TITLE) == 0;
}

void Playlist::addReturnEntry() {
    PlaylistEntry entry{};
    entry.unplayable = false;
    strcpy(entry.title, RETURN_ENTRY_TITLE);
    entries.emplace(entries.begin(), entry);
}


PlaylistEntry *Playlist::getCurrentEntry() const {
    return filteredEntries[selectedPosition];
}

std::vector<PlaylistEntry *> Playlist::getWindow() {
    return window;
}

bool Playlist::isAtLastEntry() const {
    return selectedPosition == getSize() - 1;
}

char *Playlist::getSearchableText(const int index) {
    return entries[index].getName();
}

void Playlist::markAsFound(const int index, const char position) {
    entries[index].foundStart = position + 1;
}

void Playlist::unmarkAsFound(const int index) {
    entries[index].foundStart = 0;
}

void Playlist::sort() {
    std::sort(entries.begin(), entries.end(), [this](const PlaylistEntry &a, const PlaylistEntry &b) {
        if (strcmp(a.title, RETURN_ENTRY_TITLE) == 0) return true;
        if (strcmp(b.title, RETURN_ENTRY_TITLE) == 0) return false;
        return strcasecmp(a.title, b.title) < 0;
    });
}

void Playlist::getFullPathForSelectedEntry(TCHAR *fullPath, const size_t size) const {
    snprintf(fullPath, size, "%s/%s", name, getCurrentEntry()->fileName);
}

void Playlist::tryToAddAsPsid(FILINFO *fileInfo) {
    FIL pFile;
    BYTE header[PSID_MINIMAL_HEADER_SIZE];
    UINT bytesRead;
    TCHAR fullPath[MAX_PATH_LENGTH];
    snprintf(fullPath, MAX_PATH_LENGTH, "%s/%s", name, fileInfo->fname);
    f_open(&pFile, fullPath, FA_READ);
    f_read(&pFile, &header, PSID_MINIMAL_HEADER_SIZE, &bytesRead);
    if (bytesRead == PSID_MINIMAL_HEADER_SIZE) {
        if (const uint32_t magic = header[3] | header[2] << 0x08 | header[1] << 0x10 | header[0] << 0x18;
            magic == PSID_ID || magic == RSID_ID) {
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

void Playlist::markCurrentEntryAsUnplayable() const {
    getCurrentEntry()->unplayable = true;
}
