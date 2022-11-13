#ifndef SIDPOD_PSID_CATALOG_H
#define SIDPOD_PSID_CATALOG_H

#include <stdbool.h>
#include "ff.h"

#define PSID_ID 0x50534944;

void index_psid_files();
bool is_PSID_file(FILINFO filinfo);

#endif //SIDPOD_PSID_CATALOG_H
