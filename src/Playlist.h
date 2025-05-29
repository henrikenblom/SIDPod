#ifndef SIDPOD_PLAYLIST_H
#define SIDPOD_PLAYLIST_H

#include <vector>
#include "ff.h"
#include "platform_config.h"

struct PlaylistEntry {
    bool unplayable;
    TCHAR fileName[FF_SFN_BUF + 1];
    char title[32];
    char author[32];
    bool selected;

    [[nodiscard]] char *getName() const {
        static char name[65];
        if (author[0] != '\0') {
            snprintf(name, sizeof(name), "%s - %s", title, author);
        } else {
            snprintf(name, sizeof(name), "%s", title);
        }
        return name;
    }
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

    size_t getSize() const;

    std::vector<PlaylistEntry *> getWindow();

    bool isAtLastEntry() const;

    void selectNext();

    void selectPrevious();

    void selectFirst();

    void selectLast();

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

    void enableFind();

    void disableFind();

    bool findIsEnabled() const;

    void addToSearchString(char c);

    void clearSearchString();

    char *getSearchString();

private:
    std::vector<PlaylistEntry> entries = std::vector<PlaylistEntry>(MAX_LIST_ENTRIES);
    std::vector<PlaylistEntry *> window;
    uint8_t windowPosition = 0;
    uint8_t selectedPosition = 0;
    uint8_t windowSize = CATALOG_WINDOW_SIZE;
    const char *name;
    char searchString[9] = {};
    bool findEnabled = false;
    State state = OUTDATED;

    void tryToAddAsPsid(FILINFO *fileInfo);

    static bool isRegularFile(const FILINFO *fileInfo);;

    void findSearchString();

    void updateWindow();

    void slideDown();

    void slideUp();
};

#endif //SIDPOD_PLAYLIST_H
