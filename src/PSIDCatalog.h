#ifndef SIDPOD_PSIDCATALOG_H
#define SIDPOD_PSIDCATALOG_H

#include <vector>
#include "ff.h"

struct CatalogEntry {
    bool unplayable;
    TCHAR fileName[FF_SFN_BUF + 1];
    char title[32];
    bool selected;
};

class PSIDCatalog {

public:
    static void refresh();

    static CatalogEntry *getCurrentEntry();

    static size_t getSize();

    static std::vector<CatalogEntry *> getWindow();

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

#endif //SIDPOD_PSIDCATALOG_H
