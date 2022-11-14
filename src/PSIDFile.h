#ifndef SIDPOD_PSIDFILE_H
#define SIDPOD_PSIDFILE_H


#include "ff.h"

class PSIDFile {
public:
    FILINFO fileInfo{};
    char title[32]{};
    char author[32]{};
    char released[32]{};

    explicit PSIDFile(FILINFO _fileInfo) {
        fileInfo = _fileInfo;
    }
};


#endif //SIDPOD_PSIDFILE_H
