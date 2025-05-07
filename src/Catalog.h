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

    static std::vector<std::string *> getWindow();

    static bool playlistIsOpen();

    static void closeSelected();

    static void openSelected();

    static void slideDown();

    static void slideUp();

    static void selectNext();

    static void selectPrevious();

    static void selectLast();

    static size_t getSize();

    static Playlist *getCurrentPlaylist();

    static void resetAccessors();

private:
    static bool isValidDirectory(FILINFO *fileInfo) {
        return fileInfo->fattrib == AM_DIR && fileInfo->fname[0] != 46 && containsAtLeastOnePSID(fileInfo->fname);
    }

    static bool isPSID(char *path);

    static bool containsAtLeastOnePSID(char *name);

    static void updateWindow();
};


#endif //CATALOG_H
