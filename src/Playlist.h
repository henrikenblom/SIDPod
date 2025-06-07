#ifndef SIDPOD_PLAYLIST_H
#define SIDPOD_PLAYLIST_H

#include <vector>
#include <cstdio>

#include "EntryBase.h"
#include "ff.h"
#include "ListViewBase.h"

struct PlaylistEntry final : EntryBase {
    bool unplayable;
    TCHAR fileName[FF_SFN_BUF + 1];
    char title[32];
    char author[32];

    [[nodiscard]] char *getName() const {
        static char name[67];
        if (author[0] != '\0') {
            snprintf(name, sizeof(name), "%s - %s", title, author);
        } else {
            snprintf(name, sizeof(name), "%s", title);
        }
        return name;
    }
};

class Playlist final : public ListViewBase<PlaylistEntry> {
public:
    explicit Playlist(const char *name) {
        this->name = name;
    }

    [[nodiscard]] PlaylistEntry *getCurrentEntry() const;

    std::vector<PlaylistEntry *> getWindow();

    [[nodiscard]] bool isAtLastEntry() const;

    void markCurrentEntryAsUnplayable() const;

    int initRefresh() override;

    bool advanceRefresh() override;

    [[nodiscard]] const char *getName() const;

    [[nodiscard]] bool isAtReturnEntry() const;

    void addReturnEntry();

    void getFullPathForSelectedEntry(TCHAR *fullPath, size_t size) const;

private:
    const char *name;

    void tryToAddAsPsid(FILINFO *fileInfo);

    static bool isRegularFile(const FILINFO *fileInfo);;

    char *getSearchableText(int index) override;

    void markAsFound(int index, char position) override;

    void unmarkAsFound(int index) override;

    void sort() override;
};

#endif //SIDPOD_PLAYLIST_H
