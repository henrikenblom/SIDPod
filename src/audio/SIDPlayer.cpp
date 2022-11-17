#include <pico/multicore.h>
#include <cstdio>
#include <cstring>
#include "SIDPlayer.h"

static struct audio_buffer_pool *audioBufferPool;
static sid_info sidInfo{};
uint16_t intermediateBuffer[SAMPLES_PER_BUFFER];
bool rendering = false;

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

bool SIDPlayer::play() {
    if (sidInfo.play_addr != 0) {
        rendering = true;
        sidPoke(24, 15);
        cpuJSR(sidInfo.init_addr, sidInfo.start_song);
        multicore_launch_core1(sampleRenderingLoop);
        multicore_fifo_pop_blocking();
        return true;
    } else {
        return false;
    }
}

void SIDPlayer::stop() {
    // TODO Figure out if the I2S buffer can be drained somehow, to avoid glitching during song switching
    if (rendering) {
        rendering = false;
        busy_wait_ms(60);
        multicore_reset_core1();
    }
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

void SIDPlayer::sampleRenderingLoop() {
    multicore_fifo_push_blocking(AUDIO_RENDERING_STARTED);
    while (rendering) {
        struct audio_buffer *buffer = take_audio_buffer(audioBufferPool, true);
        auto *samples = (int16_t *) buffer->buffer->bytes;
        generateSamples();
        for (uint i = 0; i < buffer->max_sample_count; i++) {
            samples[i] = (int16_t) intermediateBuffer[i];
        }
        buffer->sample_count = buffer->max_sample_count;
        multicore_fifo_push_timeout_us(NEW_AUDIO_DATA_AVAILABLE, 0);
        give_audio_buffer(audioBufferPool, buffer);
    }
}

void SIDPlayer::initAudio() {
    static audio_format_t audio_format = {
            .sample_freq = SAMPLE_RATE,
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .channel_count = 1,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 2
    };

    audioBufferPool = audio_new_producer_pool(&producer_format, 2,
                                              SAMPLES_PER_BUFFER);
    const struct audio_format *output_format;
    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    audio_i2s_connect_extra(audioBufferPool, false, 1,
                            128, nullptr);
    audio_i2s_set_enabled(true);
}