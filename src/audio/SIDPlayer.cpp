#include <pico/multicore.h>
#include <cstdio>
#include <cstring>
#include <pico/util/queue.h>
#include <hardware/gpio.h>
#include "SIDPlayer.h"

#include <algorithm>
#include <mixer.h>
#include <psiddrv.h>
#include <sidemu.h>
#include <TuneInfo.h>
#include <c64/c64.h>
#include <pico/audio.h>
#include <pico/audio_i2s.h>

#include "../platform_config.h"
#include "../Catalog.h"

repeating_timer reapCommandTimer{};
queue_t txQueue;
uint8_t playPauseCommand = PLAY_PAUSE_COMMAND_CODE;
uint8_t volume = INITIAL_VOLUME;
libsidplayfp::c64 m_c64;
libsidplayfp::Mixer m_mixer;
TuneInfo *info;
uint8_t videoSwitch;
unsigned short firstSidAddr = 0xd400;
unsigned short secondSidAddr = 0;
unsigned short thirdSidAddr = 0;
uint16_t currentSong;
uint_least32_t m_startTime = 0;
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
    .dma_channel = 0,
    .pio_sm = 0,
};

// core0 functions

void SIDPlayer::initAudio() {
    updateVolumeFactor();
    m_mixer.setSamplerate(SAMPLE_RATE);
    multicore_launch_core1(core1Main);
    multicore_fifo_pop_blocking();
}

void SIDPlayer::resetState() {
    playPauseQueued = false;
    rendering = false;
    loadingSuccessful = true;
    lastCatalogEntry = {};
    memset(visualizationBuffer, 0, FFT_SAMPLES);
    initialiseC64();
}

void SIDPlayer::togglePlayPause() {
    queue_add_blocking(&txQueue, &playPauseCommand);
}

void SIDPlayer::ampOn() {
    gpio_pull_up(AMP_CONTROL_PIN);
}

void SIDPlayer::ampOff() {
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
}

bool SIDPlayer::lineLevelOn() {
    return false;
}

TuneInfo *SIDPlayer::getSidInfo() {
    return info;
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

libsidplayfp::c64::model_t SIDPlayer::c64model() {
    TuneInfo::clock_t clockSpeed = *info->clockSpeed;

    libsidplayfp::c64::model_t model;

    // Use preferred speed if forced or if song speed is unknown
    if ((clockSpeed == TuneInfo::CLOCK_UNKNOWN) || (clockSpeed == TuneInfo::CLOCK_ANY)) {
        clockSpeed = TuneInfo::CLOCK_PAL;
        model = libsidplayfp::c64::PAL_B;
        videoSwitch = 1;
    } else {
        switch (clockSpeed) {
            default:
            case TuneInfo::CLOCK_PAL:
                model = libsidplayfp::c64::PAL_B;
                videoSwitch = 1;
                break;
            case TuneInfo::CLOCK_NTSC:
                model = libsidplayfp::c64::NTSC_M;
                videoSwitch = 0;
                break;
        }
    }
    return model;
}

bool SIDPlayer::initialiseC64() {
    m_c64.reset();

    for (int i = 0; i <= 0x1FFF; i++) {
        for (int j = 0; j < 100; j++)
            m_c64.clock();
        m_mixer.clockChips();
        m_mixer.resetBufs();
    }
    libsidplayfp::psiddrv driver(info);
    if (!driver.drvReloc()) {
        printf("%s\n", driver.errorString());
        return false;
    }
    info->m_driverAddr = driver.driverAddr();
    info->m_driverLength = driver.driverLength();
    info->m_powerOnDelay = 0x1FFF;

    driver.install(m_c64.getMemInterface(), videoSwitch);

    return true;
}

void SIDPlayer::placeSidTuneInC64mem(libsidplayfp::sidmemory &mem, FIL pFile) {
    // The Basic ROM sets these values on loading a file.
    // Program end address
    const uint_least16_t start = info->load;
    const uint_least16_t end = start + info->c64dataLen;
    mem.writeMemWord(0x2d, end); // Variables start
    mem.writeMemWord(0x2f, end); // Arrays start
    mem.writeMemWord(0x31, end); // Strings start
    mem.writeMemWord(0xac, start);
    mem.writeMemWord(0xae, end);

    UINT bytesRead;
    UINT totalBytesRead = 0;
    f_lseek(&pFile, info->fileOffset);
    while (true) {
        BYTE buffer[SID_LOAD_BUFFER_SIZE];
        f_read(&pFile, buffer, SID_LOAD_BUFFER_SIZE, &bytesRead);
        if (bytesRead == 0) break;
        mem.fillRam(info->load + totalBytesRead, buffer, bytesRead);
        totalBytesRead += bytesRead;
    }
    f_close(&pFile);
}

void SIDPlayer::run(unsigned int events) {
    for (unsigned int i = 0; i < events; i++)
        m_c64.clock();
}

uint_least32_t SIDPlayer::play(short *buffer, uint_least32_t count) {
    static constexpr unsigned int CYCLES = 3000;

    m_mixer.begin(buffer, count);

    while (m_mixer.notFinished()) {
        if (!m_mixer.wait())
            run(CYCLES);

        m_mixer.clockChips();
        m_mixer.doMix();
    }
    return m_mixer.samplesGenerated();
}

volatile bool SIDPlayer::loadPSID(TCHAR *file_name) {
    FIL pFile;
    BYTE header[SID_HEADER_SIZE];
    UINT bytesRead;
    if (f_open(&pFile, file_name, FA_READ) != FR_OK) return false;
    if (f_read(&pFile, &header, SID_HEADER_SIZE, &bytesRead) != FR_OK) return false;
    if (bytesRead < SID_HEADER_SIZE) return false;

    info = new TuneInfo(header, SID_HEADER_SIZE, pFile);

    libsidplayfp::sidemu *sid;
    sid->lock(m_c64.getEventScheduler());
    sid->model(MOS8580, 0);
    m_c64.setBaseSid(sid);
    m_mixer.addSid(sid);

    placeSidTuneInC64mem(m_c64.getMemInterface(), pFile);

    m_c64.resetCpu();
    m_startTime = m_c64.getTimeMs();

    return true;
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
            if (Catalog::getCurrentPlaylist() != nullptr) {
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
            auto *samples = reinterpret_cast<int16_t *>(buffer->buffer->bytes);
            buffer->sample_count = play(samples, MAX_SAMPLES_PER_BUFFER);
            give_audio_buffer(audioBufferPool, buffer);
        }
    }
}
