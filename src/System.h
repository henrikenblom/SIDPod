#ifndef SIDPOD_SYSTEM_H
#define SIDPOD_SYSTEM_H
#include "ff.h"

class System {
public:
    static void goDormant();

    static void softReset();

    static void hardReset();

    static void virtualVBLSync();

    static void enableUsb();

    static bool usbConnected();

    static void deleteSettingsFile(const char *fileName);

    static bool openSettingsFile(FIL *fil, const char *fileName);

    static bool mountAndPrepareFilesystem();

private:
    static bool repeatingTudTask(struct repeating_timer *t);

    static bool createSettingsDirectoryIfNotExists();
};

#endif //SIDPOD_SYSTEM_H
