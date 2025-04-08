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

repeating_timer reapCommandTimer{};
queue_t txQueue;
uint8_t playPauseCommand = PLAY_PAUSE_COMMAND_CODE;
uint8_t volume = INITIAL_VOLUME;
float volumeFactor;
short visualizationBuffer[FFT_SAMPLES];
volatile bool playPauseQueued = false;
bool rendering = false;
bool loadingSuccessful = true;
CatalogEntry *lastCatalogEntry = {};
static audio_format_t audio_format = {
    .sample_freq = SAMPLE_RATE,
    .format = AUDIO_BUFFER_FORMAT_PCM_S16,
    .channel_count = 1,
};
static audio_buffer_format producer_format = {
    .format = &audio_format,
    .sample_stride = 2
};
static audio_buffer_pool *audioBufferPool = audio_new_producer_pool(&producer_format, 2, SAMPLES_PER_BUFFER);
audio_i2s_config config = {
    .data_pin = PICO_AUDIO_I2S_DATA_PIN,
    .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
    .dma_channel = 0,
    .pio_sm = 0,
};

// core0 functions

void SIDPlayer::initAudio() {
    updateVolumeFactor();
    multicore_launch_core1(core1Main);
    multicore_fifo_pop_blocking();
}

void SIDPlayer::resetState() {
    playPauseQueued = false;
    rendering = false;
    loadingSuccessful = true;
    lastCatalogEntry = {};
    memset(visualizationBuffer, 0, SAMPLES_PER_BUFFER);
    C64::c64Init();
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

CatalogEntry *SIDPlayer::getCurrentlyLoaded() {
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

sid_info *SIDPlayer::getSidInfo() {
    return C64::getSidInfo();
}

bool SIDPlayer::loadingWasSuccessful() {
    return loadingSuccessful;
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

volatile bool SIDPlayer::loadPSID(CatalogEntry *sidFile) {
    return C64::sid_load_from_file(sidFile->fileName);
}

[[noreturn]] void SIDPlayer::core1Main() {
    ampOff();
    audio_i2s_setup(&audio_format, &config);
    audio_i2s_connect(audioBufferPool);
    audio_i2s_set_enabled(true);
    queue_init(&txQueue, 1, 1);
    add_repeating_timer_ms(50, reinterpret_cast<repeating_timer_callback_t>(reapCommand), nullptr, &reapCommandTimer);
    multicore_fifo_push_blocking(AUDIO_RENDERING_STARTED_FIFO_FLAG);
    while (true) {
        if (playPauseQueued) {
            CatalogEntry *currentCatalogEntry = PSIDCatalog::getCurrentEntry();
            if (strcmp(currentCatalogEntry->fileName, lastCatalogEntry->fileName) != 0) {
                resetState();
                if (loadPSID(PSIDCatalog::getCurrentEntry())) {
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

        if (rendering) {
            audio_buffer *buffer = take_audio_buffer(audioBufferPool, true);
            C64::generateSamples(buffer, volumeFactor);
            give_audio_buffer(audioBufferPool, buffer);
        }
    }
}
