#ifndef SIDPOD_PSIDCATALOG_H
#define SIDPOD_PSIDCATALOG_H

#include "ff.h"
#include "PSIDFile.h"

class PSIDCatalog {

public:
    static void refreshCatalog();
    static PSIDFile getNextPsidFile();
    static size_t getSize();
private:
    static void tryToAddAsPsid(FILINFO fileInfo);
};

#endif //SIDPOD_PSIDCATALOG_H
