#ifndef SIDPOD_PLAYLIST_H
#define SIDPOD_PLAYLIST_H

#include <vector>
#include "ff.h"

struct PlaylistEntry {
    bool unplayable;
    TCHAR fileName[FF_SFN_BUF + 1];
    char title[32];
    bool selected;
};

class Playlist {

public:
    static void refresh();

    static PlaylistEntry *getCurrentEntry();

    static size_t getSize();

    static std::vector<PlaylistEntry *> getWindow();

    static void selectNext();

    static void selectPrevious();

    static void markCurrentEntryAsUnplayable();

private:
    static void tryToAddAsPsid(FILINFO *fileInfo);

    static bool isRegularFile(FILINFO *fileInfo);

    static void resetAccessors();

    static void updateWindow();

    static void slideDown();

    static void slideUp();
};

#endif //SIDPOD_PLAYLIST_H
