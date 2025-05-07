#ifndef SIDPOD_SIDPLAYER_H
#define SIDPOD_SIDPLAYER_H

#include <pico/audio.h>

#include "C64.h"
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

    static SidInfo *getSidInfo();

    static bool loadingWasSuccessful();

    static int getCurrentSong();

    static int getSongCount();

    static void playNextSong();

    static void playPreviousSong();

    static uint32_t millisSinceSongStart();

    static void resetState();

    static void playIfPaused();

    static void pauseIfPlaying();

private:
    static volatile void tryJSRToPlayAddr();

    static void updateVolumeFactor();

    static volatile void generateSamples(audio_buffer *buffer);

    [[noreturn]] static void core1Main();

    static volatile bool reapCommand(struct repeating_timer *t);
};

#endif //SIDPOD_SIDPLAYER_H
