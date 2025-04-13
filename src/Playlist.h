#ifndef SIDPOD_PLAYLIST_H
#define SIDPOD_PLAYLIST_H

#include <vector>
#include "ff.h"
#include "platform_config.h"

struct PlaylistEntry {
    bool unplayable;
    TCHAR fileName[FF_SFN_BUF + 1];
    char title[32];
    bool selected;
};

class Playlist {
public:
    explicit Playlist(TCHAR *name) {
        this->name = name;
        refresh();
    }

    ~Playlist() {
        entries.clear();
        window.clear();
    }

    PlaylistEntry *getCurrentEntry();

    size_t getSize();

    std::vector<PlaylistEntry *> getWindow();

    void selectNext();

    void selectPrevious();

    void markCurrentEntryAsUnplayable();

    [[nodiscard]] bool isReady() const {
        return ready;
    }

    void refresh();

    TCHAR *getName() const;

    bool isAtReturnEntry() const;

    void addReturnEntry();

    void getFullPathForSelectedEntry(TCHAR *fullPath);

private:
    std::vector<PlaylistEntry> entries = std::vector<PlaylistEntry>(MAX_PLAYLIST_ENTRIES);
    std::vector<PlaylistEntry *> window;
    uint8_t windowPosition = 0;
    uint8_t selectedPosition = 0;
    uint8_t windowSize = CATALOG_WINDOW_SIZE;
    TCHAR *name;
    bool ready = false;

    void tryToAddAsPsid(FILINFO *fileInfo);

    bool isRegularFile(FILINFO *fileInfo);

    void resetAccessors();

    void updateWindow();

    void slideDown();

    void slideUp();
};

#endif //SIDPOD_PLAYLIST_H
