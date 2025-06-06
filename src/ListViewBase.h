//
// Created by Henrik Enblom on 2025-05-31.
//

#ifndef LISTVIEWBASE_H
#define LISTVIEWBASE_H
#include <algorithm>
#include <vector>

#include "platform_config.h"

template<typename EntryType>
class ListViewBase {
protected:
    std::vector<EntryType> entries;
    std::vector<EntryType *> window;
    size_t selectedPosition = 0;
    size_t windowPosition = 0;
    char searchTerm[9] = {};
    bool findEnabled = false;

    virtual void refresh() = 0;

    virtual char *getSearchableText(int index) = 0;

    virtual ~ListViewBase() {
        entries.clear();
        window.clear();
    }

    void updateWindow() {
        if (getSize()) {
            window.clear();
            for (int i = 0; i < std::min(LIST_WINDOW_SIZE, getSize()); i++) {
                auto entry = &entries.at(windowPosition + i);
                entry->selected = windowPosition + i == selectedPosition;
                window.push_back(entry);
            }
        }
    }

    void slideDown() {
        if (LIST_WINDOW_SIZE + windowPosition < getSize()) {
            windowPosition++;
        }
    }

    void slideUp() {
        if (windowPosition > 0) {
            windowPosition--;
        }
    }

    void findSearchTerm() {
        if (searchTerm[0] == '\0') {
            return;
        }
        for (size_t i = 0; i < entries.size(); ++i) {
            if (strncasecmp(getSearchableText(i), searchTerm, strlen(searchTerm)) == 0) {
                selectedPosition = i;
                windowPosition = std::min(i, entries.size() - LIST_WINDOW_SIZE);
                updateWindow();
                break;
            }
        }
    }

public:
    [[nodiscard]] size_t getSize() const { return entries.size(); }

    void selectNext() {
        if (selectedPosition < getSize() - 1) {
            selectedPosition++;
            slideDown();
            updateWindow();
        }
    }

    void selectPrevious() {
        if (selectedPosition > 0) {
            selectedPosition--;
            slideUp();
            updateWindow();
        }
    }

    void selectFirst() {
        selectedPosition = 0;
        windowPosition = 0;
        updateWindow();
    }

    void selectLast() {
        selectedPosition = getSize() - 1;
        windowPosition = getSize() > LIST_WINDOW_SIZE ? getSize() - LIST_WINDOW_SIZE : 0;
        updateWindow();
    }

    void resetAccessors() {
        std::sort(entries.begin(), entries.end());
        selectedPosition = 0;
        windowPosition = 0;
        if (getSize() > 0) {
            updateWindow();
        }
    }

    std::vector<EntryType *> getWindow() const {
        return window;
    }

    void enableFind() {
        findEnabled = true;
    }

    void disableFind() {
        findEnabled = false;
        clearSearchTerm();
        updateWindow();
    }

    [[nodiscard]] bool findIsEnabled() const {
        return findEnabled;
    }

    void addToSearchTerm(char c) {
        if (const size_t len = strlen(searchTerm); len < sizeof(searchTerm) - 1) {
            searchTerm[len] = c;
            searchTerm[len + 1] = '\0';
        }
        findSearchTerm();
    }

    void clearSearchTerm() {
        searchTerm[0] = '\0';
    }

    char *getSearchTerm() {
        return searchTerm;
    }
};
#endif //LISTVIEWBASE_H
