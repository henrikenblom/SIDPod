#ifndef SIDPOD_PSIDCATALOG_H
#define SIDPOD_PSIDCATALOG_H

#include <vector>
#include "ff.h"
#include "PSIDCatalogEntry.h"

class PSIDCatalog {

public:
    static void refresh();

    static PSIDCatalogEntry getCurrentEntry();

    static size_t getSize();

    static std::vector<PSIDCatalogEntry> getWindow();

    static void selectNext();

    static void selectPrevious();

private:
    static void tryToAddAsPsid(FILINFO fileInfo);

    static void resetAccessors();

    static void updateWindow();

    static void slideDown();

    static void slideUp();
};

#endif //SIDPOD_PSIDCATALOG_H
