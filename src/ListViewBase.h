//
// Created by Henrik Enblom on 2025-05-31.
//

#ifndef LISTVIEWBASE_H
#define LISTVIEWBASE_H
#include "platform_config.h"

#include <vector>

#include <cctype>
#include <cstring>

inline const char *strcasestr(const char *lot, const char *term) {
    if (!*term) return lot;
    for (; *lot; ++lot) {
        const char *h = lot;
        const char *n = term;
        while (*h && *n && std::tolower(static_cast<unsigned char>(*h)) ==
               std::tolower(static_cast<unsigned char>(*n))) {
            ++h;
            ++n;
        }
        if (!*n) return lot;
    }
    return nullptr;
}

template<typename EntryType = EntryBase>
class ListViewBase {
public:
    enum State {
        OUTDATED,
        REFRESHING,
        READY,
    };

    virtual int initRefresh() = 0;

    virtual bool advanceRefresh() = 0;

    [[nodiscard]] float getRefreshProgress() const {
        return static_cast<float>(candidateIndex) / static_cast<float>(candidateCount);
    }

    [[nodiscard]] State getState() const {
        return state;
    }

    [[nodiscard]] size_t getSize() const { return entries.size(); }

    [[nodiscard]] size_t getFilteredSize() const {
        return filteredEntries.size();
    }

    [[nodiscard]] size_t getSelectedPosition() const {
        return selectedPosition;
    }

    [[nodiscard]] size_t getWindowPosition() const {
        return windowPosition;
    }

    void selectNext() {
        if (selectedPosition < filteredEntries.size() - 1) {
            selectedPosition++;
            slideDown();
            updateViews();
        }
    }

    void selectPrevious() {
        if (selectedPosition > 0) {
            selectedPosition--;
            slideUp();
            updateViews();
        }
    }

    void selectFirst() {
        selectedPosition = 0;
        windowPosition = 0;
        updateViews();
    }

    void selectLast() {
        selectedPosition = getFilteredSize() - 1;
        windowPosition = getFilteredSize() > LIST_WINDOW_SIZE ? getFilteredSize() - LIST_WINDOW_SIZE : 0;
        updateViews();
    }

    void resetAccessors() {
        sort();
        selectedPosition = 0;
        windowPosition = 0;
        if (getSize() > 0) {
            updateViews();
        }
    }

    std::vector<EntryType *> getWindow() const {
        return window;
    }

    void focusFilterInput() {
        filterInputFocused = true;
    }

    void unfocusFilterInput() {
        filterInputFocused = false;
    }

    void disableFilterInput() {
        filterInputFocused = false;
        clearFilterTerm();
        updateViews();
    }

    [[nodiscard]] bool filterInputIsFocused() const {
        return filterInputFocused;
    }

    [[nodiscard]] bool hasFilterTerm() const {
        return filterTerm[0] != '\0';
    }

    void addToFilterTerm(const char c) {
        if (const size_t len = strlen(filterTerm); len < sizeof(filterTerm) - 1) {
            filterTerm[len] = c;
            filterTerm[len + 1] = '\0';
        }
        executeFilter();
    }

    void deleteFromFilterTerm() {
        const size_t len = strlen(filterTerm);
        if (len == 0) {
            disableFilterInput();
            return;
        }
        filterTerm[len - 1] = '\0';
        if (filterTerm[0] == '\0') {
            disableFilterInput();
            return;
        }
        executeFilter();
    }

    void clearFilterTerm() {
        filterTerm[0] = '\0';
        selectedPosition = 0;
        windowPosition = 0;
        for (size_t i = 0; i < entries.size(); ++i) {
            unmarkAsFound(i);
        }
    }

    char *getFilterTerm() {
        return filterTerm;
    }

    bool selectionHasChanged() {
        if (selectionChanged) {
            selectionChanged = false;
            return true;
        }
        return false;
    }

protected:
    DIR *dp{};
    std::vector<EntryType> entries;
    std::vector<EntryType *> filteredEntries;
    std::vector<EntryType *> window;
    size_t selectedPosition = 0;
    size_t windowPosition = 0;
    int candidateCount = 0;
    int candidateIndex = 0;
    char filterTerm[13] = {};
    bool filterInputFocused = false;
    bool selectionChanged = false;
    State state = OUTDATED;

    virtual char *getSearchableText(int index) = 0;

    virtual void markAsFound(int index, char position) = 0;

    virtual void unmarkAsFound(int index) = 0;

    virtual void sort() = 0;

    virtual ~ListViewBase() {
        entries.clear();
        window.clear();
    }

    void updateViews() {
        if (getSize()) {
            filteredEntries.clear();
            int index = 0;
            for (int i = 0; i < getSize(); ++i) {
                if (EntryType *entry = &entries.at(i); !hasFilterTerm() || entry->foundStart) {
                    entry->selected = index++ == selectedPosition;
                    filteredEntries.push_back(entry);
                }
            }
            window.clear();
            for (int i = windowPosition; i < filteredEntries.size(); i++) {
                auto entry = filteredEntries[i];
                window.push_back(entry);
                if (window.size() == LIST_WINDOW_SIZE) {
                    break;
                }
            }
        }
        selectionChanged = true;
    }

    void slideDown() {
        if (LIST_WINDOW_SIZE + windowPosition < filteredEntries.size()) {
            windowPosition++;
        }
    }

    void slideUp() {
        if (windowPosition > 0) {
            windowPosition--;
        }
    }

    void executeFilter() {
        if (filterTerm[0] == '\0') {
            return;
        }
        bool foundOne = false;
        int index = 0;
        for (size_t i = 0; i < entries.size(); ++i) {
            unmarkAsFound(i);
            const char *searchableText = getSearchableText(i);
            if (const char *base = strcasestr(searchableText, filterTerm)) {
                const size_t startPos = base - searchableText;
                markAsFound(i, startPos);
                if (!foundOne) {
                    selectedPosition = index;
                    windowPosition = 0;
                    foundOne = true;
                }
                index++;
            }
        }
        updateViews();
    }
};
#endif //LISTVIEWBASE_H
