#include <pico/multicore.h>
#include <cstdio>
#include <hardware/gpio.h>
#include "SIDPlayer.h"
#include "../PSIDCatalog.h"

static sid_info sidInfo{};
uint16_t intermediateBuffer[SAMPLES_PER_BUFFER];
bool lastButtonState = false;
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

bool SIDPlayer::loadPSID(PSIDCatalogEntry psidFile) {
    FIL pFile;
    BYTE buffer[psidFile.fileInfo.fsize];
    UINT bytes_read;
    stop();
    f_open(&pFile, psidFile.fileInfo.fname, FA_READ);
    f_read(&pFile, &buffer, psidFile.fileInfo.fsize, &bytes_read);
    f_close(&pFile);
    c64Init(SAMPLE_RATE);
    return sid_load_from_memory((char *) buffer, bytes_read, &sidInfo);
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

void SIDPlayer::mainLoop() {
    audio_i2s_setup(&audio_format, &config);
    audio_i2s_connect(audioBufferPool);
    audio_i2s_set_enabled(true);
    bool rendering = false;
    bool loadingQueued = false;
    multicore_fifo_push_blocking(AUDIO_RENDERING_STARTED);
    while (true) {
        if (buttonPushed() || loadingQueued) {
            loadPSID(PSIDCatalog::getCurrentEntry());
            sidPoke(24, 15); // TODO Seems to have no effect?
            cpuJSR(sidInfo.init_addr, sidInfo.start_song);
            rendering = true;
            loadingQueued = false;
        }
        if (rendering && sidInfo.play_addr != 0) {
            struct audio_buffer *buffer = take_audio_buffer(audioBufferPool, true);
            auto *samples = (int16_t *) buffer->buffer->bytes;
            generateSamples();
            for (uint i = 0; i < buffer->max_sample_count; i++) {
                samples[i] = (int16_t) intermediateBuffer[i];
                if (buttonPushed()) {
                    loadingQueued = true;
                    printf("Load queued\n");
                }
            }
            buffer->sample_count = buffer->max_sample_count;
            give_audio_buffer(audioBufferPool, buffer);
        }
    }
}

bool SIDPlayer::buttonPushed() {
    bool currentState = !gpio_get(ENC_SW);
    bool pushed = false;
    if (currentState && currentState != lastButtonState) {
        pushed = true;
    }
    lastButtonState = currentState;
    return pushed;
}

void SIDPlayer::initAudio() {
    multicore_launch_core1(mainLoop);
    multicore_fifo_pop_blocking();
}

bool SIDPlayer::play() {
    return false;
}

void SIDPlayer::stop() {


}
