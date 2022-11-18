#ifndef SIDPOD_PSIDCATALOGENTRY_H
#define SIDPOD_PSIDCATALOGENTRY_H


#include "ff.h"

class PSIDCatalogEntry {
public:
    PSIDCatalogEntry() = default;

    FILINFO fileInfo{};
    char title[32]{};
    char author[32]{};
    char released[32]{};
    bool selected = false;

    explicit PSIDCatalogEntry(FILINFO _fileInfo) {
        fileInfo = _fileInfo;
    }
};


#endif //SIDPOD_PSIDCATALOGENTRY_H
