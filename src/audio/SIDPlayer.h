#ifndef SIDPOD_SIDPLAYER_H
#define SIDPOD_SIDPLAYER_H

#include <pico/audio.h>

#include "C64.h"
#include "../PSIDCatalog.h"

#define PLAY_PAUSE_COMMAND_CODE     123

extern short visualizationBuffer[];

class SIDPlayer {
public:
    static void initAudio();

    static volatile bool loadPSID(CatalogEntry *sidFile);

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

    static SidInfo *getSidInfo();

    static bool loadingWasSuccessful();

    static void resetState();

private:
    static volatile void tryJSRToPlayAddr();

    static void updateVolumeFactor();

    static volatile void generateSamples(audio_buffer *buffer);

    [[noreturn]] static void core1Main();

    static volatile bool reapCommand(struct repeating_timer *t);
};

#endif //SIDPOD_SIDPLAYER_H
