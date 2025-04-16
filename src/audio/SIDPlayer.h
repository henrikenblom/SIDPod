#ifndef SIDPOD_SIDPLAYER_H
#define SIDPOD_SIDPLAYER_H

#include <TuneInfo.h>
#include <c64/c64.h>
#include <pico/audio.h>

#include "../Playlist.h"

#define PLAY_PAUSE_COMMAND_CODE     123

extern short visualizationBuffer[];

class SIDPlayer {
public:
    static void initAudio();

    static volatile bool loadPSID(TCHAR *fullPath);

    static PlaylistEntry *getCurrentlyLoaded();

    static void togglePlayPause();

    static void ampOn();

    static void ampOff();

    static void volumeUp();

    static void volumeDown();

    static uint8_t getVolume();

    static bool isPlaying();

    static void toggleLineLevel();

    static bool lineLevelOn();

    static TuneInfo *getSidInfo();

    static bool loadingWasSuccessful();

    static void resetState();

private:
    static volatile void tryJSRToPlayAddr();

    static void updateVolumeFactor();

    static volatile void generateSamples(audio_buffer *buffer);

    [[noreturn]] static void core1Main();

    static volatile bool reapCommand(struct repeating_timer *t);

    libsidplayfp::c64::model_t c64model();

    static void readHeader(BYTE *buffer, TuneInfo &info);

    static void print_sid_info();

    static bool initialiseC64();

    static void placeSidTuneInC64mem(libsidplayfp::sidmemory &mem, FIL pFile);

    static void run(unsigned int events);
};

#endif //SIDPOD_SIDPLAYER_H
