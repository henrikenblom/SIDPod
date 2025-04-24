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
    enum State {
        OUTDATED,
        REFRESHING,
        READY,
    };

    explicit Playlist(const char *name) {
        this->name = name;
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

    [[nodiscard]] State getState() const {
        return state;
    }

    void refresh();

    const char *getName() const;

    bool isAtReturnEntry() const;

    void addReturnEntry();

    void getFullPathForSelectedEntry(TCHAR *fullPath);

    void resetAccessors();

private:
    std::vector<PlaylistEntry> entries = std::vector<PlaylistEntry>(MAX_PLAYLIST_ENTRIES);
    std::vector<PlaylistEntry *> window;
    uint8_t windowPosition = 0;
    uint8_t selectedPosition = 0;
    uint8_t windowSize = CATALOG_WINDOW_SIZE;
    const char *name;
    State state = OUTDATED;

    void tryToAddAsPsid(FILINFO *fileInfo);

    static bool isRegularFile(FILINFO *fileInfo);;

    void updateWindow();

    void slideDown();

    void slideUp();
};

#endif //SIDPOD_PLAYLIST_H
