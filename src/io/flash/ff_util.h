#ifndef SIDPOD_FF_UTIL_H
#define SIDPOD_FF_UTIL_H

#include "ff.h"

void filesystem_init();

bool verify_filesystem(FATFS *fs);

#endif //SIDPOD_FF_UTIL_H