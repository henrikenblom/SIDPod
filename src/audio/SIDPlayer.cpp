#include <pico/multicore.h>
#include <cstdio>
#include <cstring>
#include <pico/util/queue.h>
#include <hardware/gpio.h>
#include "SIDPlayer.h"
#include "../platform_config.h"
#include "sid.h"

struct repeating_timer reapCommandTimer{};
queue_t txQueue;
uint8_t playPauseCommand = PLAY_PAUSE_COMMAND_CODE;
uint8_t volume = VOLUME_STEPS;
static sid_info sidInfo{};
uint16_t intermediateBuffer[SAMPLES_PER_BUFFER];
bool playPauseQueued = false;
bool rendering = false;
catalogEntry *lastCatalogEntry = {};
static audio_format_t audio_format = {
        .sample_freq = SAMPLE_RATE,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 1,
};
static struct audio_buffer_format producer_format = {
        .format = &audio_format,
        .sample_stride = 2
};
static struct audio_buffer_pool *audioBufferPool = audio_new_producer_pool(&producer_format, 2, SAMPLES_PER_BUFFER);
struct audio_i2s_config config = {
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

void SIDPlayer::togglePlayPause() {
    queue_add_blocking(&txQueue, &playPauseCommand);
}


void SIDPlayer::turnAmpOn() {
    gpio_pull_up(AMP_CONTROL_PIN);
}

void SIDPlayer::turnAmpOff() {
    if (!getLineLevelOn()) {
        gpio_pull_down(AMP_CONTROL_PIN);
    }
}

void SIDPlayer::volumeUp() {
    if (volume < VOLUME_STEPS) {
        if (volume == 0) {
            turnAmpOn();
        }
        volume++;
    }
}

void SIDPlayer::volumeDown() {
    if (volume > 0) {
        if (volume == 1) {
            turnAmpOff();
        }
        volume--;
    }
}

uint8_t SIDPlayer::getVolume() {
    return volume;
}


catalogEntry *SIDPlayer::getCurrentlyLoaded() {
    return lastCatalogEntry;
}

bool SIDPlayer::isPlaying() {
    return rendering;
}

// core1 functions

bool SIDPlayer::reapCommand(struct repeating_timer *t) {
    (void) t;
    uint8_t value = 0;
    queue_try_remove(&txQueue, &value);
    if (value == playPauseCommand) {
        playPauseQueued = true;
    }
    return true;
}

bool SIDPlayer::loadPSID(catalogEntry *psidFile) {
    c64Init(SAMPLE_RATE);
    return sid_load_from_file(psidFile->fileName, &sidInfo);
}

void SIDPlayer::generateSamples() {
    int samples_rendered = 0;
    int samples_to_render = 0;

    while (samples_rendered < SAMPLES_PER_BUFFER) {
        if (samples_to_render == 0) {
            cpuJSR(sidInfo.play_addr, 0);

            int n_refresh_cia = (int) (20000 * (memory[0xdc04] | (memory[0xdc05] << 8)) / 0x4c00);
            if ((n_refresh_cia == 0) || (sidInfo.speed == 0))
                n_refresh_cia = 20000;

            samples_to_render = SAMPLE_RATE * n_refresh_cia / 1000000;
        }
        if (samples_rendered + samples_to_render > SAMPLES_PER_BUFFER) {
            sid_synth_render(intermediateBuffer + samples_rendered, SAMPLES_PER_BUFFER - samples_rendered);
            samples_to_render -= SAMPLES_PER_BUFFER - samples_rendered;
            samples_rendered = SAMPLES_PER_BUFFER;
        } else {
            sid_synth_render(intermediateBuffer + samples_rendered, samples_to_render);
            samples_rendered += samples_to_render;
            samples_to_render = 0;
        }
    }
}

[[noreturn]] void SIDPlayer::core1Main() {
    turnAmpOff();
    audio_i2s_setup(&audio_format, &config);
    audio_i2s_connect(audioBufferPool);
    audio_i2s_set_enabled(true);
    queue_init(&txQueue, 1, 1);
    add_repeating_timer_ms(50, reapCommand, nullptr, &reapCommandTimer);
    multicore_fifo_push_blocking(AUDIO_RENDERING_STARTED_FIFO_FLAG);
    while (true) {
        if (playPauseQueued) {
            catalogEntry *currentCatalogEntry = PSIDCatalog::getCurrentEntry();
            if (strcmp(currentCatalogEntry->fileName, lastCatalogEntry->fileName) != 0) {
                loadPSID(PSIDCatalog::getCurrentEntry());
                sidPoke(24, 15);
                cpuJSR(sidInfo.init_addr, sidInfo.start_song);
                lastCatalogEntry = currentCatalogEntry;
                rendering = true;
                turnAmpOn();
            } else if (rendering) {
                memset(intermediateBuffer, 0, sizeof(intermediateBuffer));
                rendering = false;
                turnAmpOff();
            } else {
                rendering = true;
                turnAmpOn();
            }
            playPauseQueued = false;
        }
        if (rendering && sidInfo.play_addr != 0) {
            float volumeFactor = (float) volume / VOLUME_STEPS;
            struct audio_buffer *buffer = take_audio_buffer(audioBufferPool, true);
            auto *samples = (int16_t *) buffer->buffer->bytes;
            generateSamples();
            for (uint i = 0; i < buffer->max_sample_count; i++) {
                samples[i] = (int16_t) ((float) intermediateBuffer[i] * volumeFactor);
            }
            buffer->sample_count = buffer->max_sample_count;
            give_audio_buffer(audioBufferPool, buffer);
        }
    }
}

void SIDPlayer::toggleLineLevel() {
    if (getLineLevelOn()) {
        setLineLevel(false);
    } else {
        setLineLevel(true);
    }
}

bool SIDPlayer::lineLevelOn() {
    return getLineLevelOn();
}

sid_info *SIDPlayer::getSidInfo() {
    return &sidInfo;
}
