//
// Created by Henrik Enblom on 2025-04-12.
//

#ifndef CATALOG_H
#define CATALOG_H

#include "ff.h"


class Catalog {
public:
    static void refresh();

private:
    static bool isValidDirectory(FILINFO *fileInfo) {
        return fileInfo->fattrib == AM_DIR && fileInfo->fname[0] != 46;
    }

    static bool containsSIDs(TCHAR *path);
};


#endif //CATALOG_H
