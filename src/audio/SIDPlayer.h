#ifndef SIDPOD_SIDPLAYER_H
#define SIDPOD_SIDPLAYER_H

#include <c64.h>

#include "pico/audio_i2s.h"
#include "../platform_config.h"
#include "../PSIDCatalog.h"

#define PLAY_PAUSE_COMMAND_CODE     123

extern short intermediateBuffer[];

class SIDPlayer {
public:
    static void initAudio();

    static bool loadPSID(CatalogEntry *psidFile);

    static CatalogEntry *getCurrentlyLoaded();

    static void togglePlayPause();

    static void ampOn();

    static void ampOff();

    static void volumeUp();

    static void volumeDown();

    static uint8_t getVolume();

    static bool isPlaying();

    static void toggleLineLevel();

    static bool lineLevelOn();

    static sid_info * getSidInfo();

    static bool loadingWasSuccessful();

    static void resetState();

private:

    static void generateSamples();

    [[noreturn]] static void core1Main();

    static bool reapCommand(struct repeating_timer *t);
};

#endif //SIDPOD_SIDPLAYER_H
