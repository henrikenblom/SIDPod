#ifndef SIDPOD_SYSTEM_H
#define SIDPOD_SYSTEM_H
#include "Catalog.h"
#include "ff.h"

class System {
public:
    static uint32_t millis_now();

    static void goDormant();

    static void softReset();

    static void hardReset();

    static void virtualVBLSync();

    static void enableUsb();

    static bool usbConnected();

    static void deleteSettingsFile(const char *fileName);

    static bool openSettingsFile(FIL *fil, const char *fileName);

    static bool prepareFilesystem();

private:
    // ReSharper disable once CppRedundantElaboratedTypeSpecifier
    static bool repeatingTudTask(struct repeating_timer *t);

    static bool createSettingsDirectoryIfNotExists();
};

#endif //SIDPOD_SYSTEM_H
