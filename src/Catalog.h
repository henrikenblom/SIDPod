//
// Created by Henrik Enblom on 2025-04-12.
//

#ifndef CATALOG_H
#define CATALOG_H

#include <cstring>

#include "EntryBase.h"
#include "ff.h"
#include "Playlist.h"

struct CatalogEntry final : EntryBase {
    TCHAR name[FF_LFN_BUF + 1];
    bool playing;
};

#pragma once
class Catalog;
extern Catalog *catalog;

class Catalog final : public ListViewBase<CatalogEntry> {
public:
    int initRefresh() override;

    bool advanceRefresh() override;

    void setSelectedPlaying();


    [[nodiscard]] bool hasOpenPlaylist() const;

    void closeSelected();

    void openSelected();

    [[nodiscard]] Playlist *getCurrentPlaylist() const;

private:
    Playlist *currentPlaylist = nullptr;

    static bool isValidDirectory(FILINFO *fileInfo);

    static bool isPSID(const char *fullPath);

    static bool containsAtLeastOnePSID(char *name);

    char *getSearchableText(int index) override;

    void markAsFound(int index, char position) override;

    void unmarkAsFound(int index) override;

    void sort() override;
};

#endif //CATALOG_H
