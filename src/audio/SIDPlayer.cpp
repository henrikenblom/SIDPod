#include <pico/multicore.h>
#include <cstdio>
#include <cstring>
#include <pico/util/queue.h>
#include <hardware/gpio.h>
#include "SIDPlayer.h"

#include <pico/audio.h>
#include <pico/audio_i2s.h>

#include "../platform_config.h"
#include "C64.h"

repeating_timer reapCommandTimer{};
queue_t txQueue;
uint8_t playPauseCommand = PLAY_PAUSE_COMMAND_CODE;
uint8_t volume = VOLUME_STEPS;
static sid_info sidInfo{};
short intermediateBuffer[SAMPLES_PER_BUFFER];
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
    multicore_launch_core1(core1Main);
    multicore_fifo_pop_blocking();
}

void SIDPlayer::resetState() {
    playPauseQueued = false;
    rendering = false;
    loadingSuccessful = true;
    lastCatalogEntry = {};
    memset(intermediateBuffer, 0, SAMPLES_PER_BUFFER);
    C64::c64Init(SAMPLE_RATE);
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

void SIDPlayer::volumeUp() {
    if (volume < VOLUME_STEPS) {
        if (volume == 0) {
            ampOn();
        }
        volume++;
    }
}

void SIDPlayer::volumeDown() {
    if (volume > 0) {
        if (volume == 1) {
            ampOff();
        }
        volume--;
    }
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
    return &sidInfo;
}

bool SIDPlayer::loadingWasSuccessful() {
    return loadingSuccessful;
}

// core1 functions

bool SIDPlayer::reapCommand(repeating_timer *t) {
    (void) t;
    uint8_t value = 0;
    queue_try_remove(&txQueue, &value);
    if (value == playPauseCommand) {
        playPauseQueued = true;
    }
    return true;
}

bool SIDPlayer::loadPSID(CatalogEntry *psidFile) {
    return C64::sid_load_from_file(psidFile->fileName, &sidInfo);
}

// ReSharper disable once CppDFAUnreachableFunctionCall
void SIDPlayer::generateSamples() {
    int samples_rendered = 0;
    int samples_to_render = 0;

    while (samples_rendered < SAMPLES_PER_BUFFER) {
        if (samples_to_render == 0) {
            C64::cpuJSR(sidInfo.play_addr, 0);

            int n_refresh_cia = 20000 * (memory[0xdc04] | memory[0xdc05] << 8) / 0x4c00;
            if ((n_refresh_cia == 0) || (sidInfo.speed == 0))
                n_refresh_cia = 20000;

            samples_to_render = SAMPLE_RATE * n_refresh_cia / 985248;
        }
        if (samples_rendered + samples_to_render > SAMPLES_PER_BUFFER) {
            C64::sid_synth_render(intermediateBuffer + samples_rendered, SAMPLES_PER_BUFFER - samples_rendered);
            samples_to_render -= SAMPLES_PER_BUFFER - samples_rendered;
            samples_rendered = SAMPLES_PER_BUFFER;
        } else {
            C64::sid_synth_render(intermediateBuffer + samples_rendered, samples_to_render);
            samples_rendered += samples_to_render;
            samples_to_render = 0;
        }
    }
}

// ReSharper disable once CppDFAUnreachableFunctionCall
void SIDPlayer::fillAudioBuffer(audio_buffer *buffer) {
    float volumeFactor = static_cast<float>(volume) / VOLUME_STEPS;
    volumeFactor = volumeFactor * volumeFactor;
    auto *samples = reinterpret_cast<int16_t *>(buffer->buffer->bytes);
    for (uint i = 0; i < buffer->max_sample_count; i++) {
        samples[i] = static_cast<int16_t>((float) intermediateBuffer[i] * volumeFactor);
    }
    buffer->sample_count = buffer->max_sample_count;
}

[[noreturn]] void SIDPlayer::core1Main() {
    ampOff();
    audio_i2s_setup(&audio_format, &config);
    audio_i2s_connect(audioBufferPool);
    audio_i2s_set_enabled(true);
    queue_init(&txQueue, 1, 1);
    add_repeating_timer_ms(50, reapCommand, nullptr, &reapCommandTimer);
    multicore_fifo_push_blocking(AUDIO_RENDERING_STARTED_FIFO_FLAG);
    bool firstBuffer = true;
    audio_buffer *bufferOne;
    audio_buffer *bufferTwo = take_audio_buffer(audioBufferPool, true);
    while (true) {
        if (playPauseQueued) {
            CatalogEntry *currentCatalogEntry = PSIDCatalog::getCurrentEntry();
            if (strcmp(currentCatalogEntry->fileName, lastCatalogEntry->fileName) != 0) {
                resetState();
                if (loadPSID(PSIDCatalog::getCurrentEntry())) {
                    loadingSuccessful = true;
                    C64::sidPoke(24, 15);
                    C64::cpuJSR(sidInfo.init_addr, sidInfo.start_song);
                    rendering = true;
                    ampOn();
                } else {
                    loadingSuccessful = false;
                }
                lastCatalogEntry = currentCatalogEntry;
            } else if (rendering) {
                memset(intermediateBuffer, 0, sizeof(intermediateBuffer));
                rendering = false;
                ampOff();
            } else {
                rendering = true;
                ampOn();
            }
            playPauseQueued = false;
        }

        if (rendering && sidInfo.play_addr != 0) {
            generateSamples();
            if (firstBuffer) {
                firstBuffer = false;
                bufferOne = take_audio_buffer(audioBufferPool, true);
                fillAudioBuffer(bufferOne);
                give_audio_buffer(audioBufferPool, bufferTwo);
            } else {
                firstBuffer = true;
                bufferTwo = take_audio_buffer(audioBufferPool, true);
                fillAudioBuffer(bufferTwo);
                give_audio_buffer(audioBufferPool, bufferOne);
            }
        }
    }
}
