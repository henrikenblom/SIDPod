//
// Created by Henrik Enblom on 2025-04-12.
//

#ifndef CATALOG_H
#define CATALOG_H

#include <vector>
#include <bits/basic_string.h>

#include "ff.h"
#include "Playlist.h"

class Catalog {
public:
    static void refresh();

    static std::string getPlaying();

    static void setPlaying(const std::string &playing);

    static std::string getSelected();

    static std::vector<std::string> *getEntries();

    static bool playlistIsOpen();

    static void closeSelected();

    static void openSelected();

    static void slideDown();

    static void slideUp();

    static void selectNext();

    static void selectPrevious();

    static Playlist *getCurrentPlaylist();

private:
    static bool isValidDirectory(FILINFO *fileInfo) {
        return fileInfo->fattrib == AM_DIR && fileInfo->fname[0] != 46;
    }

    static bool containsSIDs(TCHAR *path);
};


#endif //CATALOG_H
