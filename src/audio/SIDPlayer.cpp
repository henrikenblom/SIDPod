#include <pico/multicore.h>
#include <cstdio>
#include <cstring>
#include <pico/util/queue.h>
#include <hardware/gpio.h>
#include "SIDPlayer.h"

#include <algorithm>
#include <pico/audio.h>
#include <pico/audio_i2s.h>

#include "../platform_config.h"
#include "C64.h"
#include "../Catalog.h"

repeating_timer reapCommandTimer{};
queue_t txQueue;
uint8_t playPauseCommand = PLAY_PAUSE_COMMAND_CODE;
uint8_t volume = INITIAL_VOLUME;
float volumeFactor;
short visualizationBuffer[FFT_SAMPLES];
volatile bool playPauseQueued = false;
bool rendering = false;
bool loadingSuccessful = true;
PlaylistEntry *lastCatalogEntry = {};
static audio_format_t audio_format = {
    .sample_freq = SAMPLE_RATE,
    .format = AUDIO_BUFFER_FORMAT_PCM_S16,
    .channel_count = 1,
};
static audio_buffer_format producer_format = {
    .format = &audio_format,
    .sample_stride = 2
};
static audio_buffer_pool *audioBufferPool = audio_new_producer_pool(&producer_format, 2, MAX_SAMPLES_PER_BUFFER);
audio_i2s_config config = {
    .data_pin = PICO_AUDIO_I2S_DATA_PIN,
    .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
    .dma_channel = 2,
    .pio_sm = 0,
};

// core0 functions

void SIDPlayer::initAudio() {
    updateVolumeFactor();
    audio_i2s_setup(&audio_format, &config);
    audio_i2s_connect(audioBufferPool);
    audio_i2s_set_enabled(true);
    C64::begin();
    multicore_launch_core1(core1Main);
    multicore_fifo_pop_blocking();
}

void SIDPlayer::resetState() {
    playPauseQueued = false;
    rendering = false;
    loadingSuccessful = true;
    lastCatalogEntry = {};
    memset(visualizationBuffer, 0, FFT_SAMPLES);
    C64::c64Init();
    busy_wait_ms(200);
}

void SIDPlayer::playIfPaused() {
    if (!rendering) {
        togglePlayPause();
    }
}

void SIDPlayer::pauseIfPlaying() {
    if (rendering) {
        togglePlayPause();
    }
}

void SIDPlayer::togglePlayPause() {
    queue_add_blocking(&txQueue, &playPauseCommand);
}

void SIDPlayer::ampOn() {
    gpio_pull_up(AMP_CONTROL_PIN);
}

void SIDPlayer::ampOff() {
    if (!C64::getLineLevelOn()) {
        gpio_pull_down(AMP_CONTROL_PIN);
    }
}

void SIDPlayer::updateVolumeFactor() {
    volumeFactor = static_cast<float>(volume) / VOLUME_STEPS;
}

void SIDPlayer::volumeUp() {
    if (volume < VOLUME_STEPS) {
        if (volume == 0) {
            ampOn();
        }
        volume++;
    }
    updateVolumeFactor();
}

void SIDPlayer::volumeDown() {
    if (volume > 0) {
        if (volume == 1) {
            ampOff();
        }
        volume--;
    }
    updateVolumeFactor();
}

uint8_t SIDPlayer::getVolume() {
    return volume;
}

PlaylistEntry *SIDPlayer::getCurrentlyLoaded() {
    return lastCatalogEntry;
}

bool SIDPlayer::isPlaying() {
    return rendering;
}

void SIDPlayer::toggleLineLevel() {
    if (C64::getLineLevelOn()) {
        C64::setLineLevel(false);
    } else {
        C64::setLineLevel(true);
    }
}

bool SIDPlayer::lineLevelOn() {
    return C64::getLineLevelOn();
}

SidInfo *SIDPlayer::getSidInfo() {
    return C64::getSidInfo();
}

bool SIDPlayer::loadingWasSuccessful() {
    return loadingSuccessful;
}

int SIDPlayer::getCurrentSong() {
    return C64::getCurrentSong();
}

int SIDPlayer::getSongCount() {
    return C64::getSidInfo()->songs;
}

void SIDPlayer::playNextSong() {
    if (C64::songIsLoaded()) {
        int song = getCurrentSong();
        if (song++ >= getSongCount() - 1) {
            song = 0;
        }
        C64::playSong(song);
        playIfPaused();
    }
}

void SIDPlayer::playPreviousSong() {
    if (C64::songIsLoaded()) {
        int song = getCurrentSong();
        if (millisSinceSongStart() < SONG_SKIP_TIME_MS) {
            if (--song < 0) {
                song = getSongCount() - 1;
            }
        }
        C64::playSong(song);
        playIfPaused();
    }
}

uint32_t SIDPlayer::millisSinceSongStart() {
    return C64::millisSinceSongStart();
}

// core1 functions

volatile bool SIDPlayer::reapCommand(repeating_timer *t) {
    (void) t;
    uint8_t value = 0;
    queue_try_remove(&txQueue, &value);
    if (value == playPauseCommand) {
        playPauseQueued = true;
    }
    return true;
}

volatile bool SIDPlayer::loadPSID(TCHAR *fullPath) {
    return C64::sid_load_from_file(fullPath);
}

[[noreturn]] void SIDPlayer::core1Main() {
    ampOff();
    queue_init(&txQueue, 1, 1);
    add_repeating_timer_ms(50, reinterpret_cast<repeating_timer_callback_t>(reapCommand), nullptr, &reapCommandTimer);
    multicore_fifo_push_blocking(AUDIO_RENDERING_STARTED_FIFO_FLAG);
    while (true) {
        if (playPauseQueued) {
            if (Catalog::playlistIsOpen()) {
                Playlist *playlist = Catalog::getCurrentPlaylist();
                PlaylistEntry *currentCatalogEntry = playlist->getCurrentEntry();
                if (strcmp(currentCatalogEntry->fileName, lastCatalogEntry->fileName) != 0) {
                    resetState();
                    TCHAR fullPath[FF_LFN_BUF + 1];
                    playlist->getFullPathForSelectedEntry(fullPath);
                    if (loadPSID(fullPath)) {
                        Catalog::setPlaying(playlist->getName());
                        loadingSuccessful = true;
                        rendering = true;
                        ampOn();
                    } else {
                        loadingSuccessful = false;
                    }
                    lastCatalogEntry = currentCatalogEntry;
                } else if (rendering) {
                    memset(visualizationBuffer, 0, sizeof(visualizationBuffer));
                    rendering = false;
                    ampOff();
                } else {
                    rendering = true;
                    ampOn();
                }
                playPauseQueued = false;
            }
        }

        if (rendering) {
            audio_buffer *buffer = take_audio_buffer(audioBufferPool, true);
            C64::clock(buffer, volumeFactor);
            give_audio_buffer(audioBufferPool, buffer);
        }
    }
}
