//
// Created by Henrik Enblom on 2025-04-12.
//

#ifndef CATALOG_H
#define CATALOG_H

#include "ff.h"


class Catalog {

    static  bool isValidDirectory(FILINFO *fileInfo) {
        return fileInfo->fattrib == 16;
    }

public:
    static void refresh();

};



#endif //CATALOG_H
