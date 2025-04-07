#include <pico/multicore.h>
#include <cstdio>
#include <cstring>
#include <pico/util/queue.h>
#include <hardware/gpio.h>
#include "SIDPlayer.h"

#include <reloc65.h>
#include <pico/audio.h>
#include <pico/audio_i2s.h>

#include "../platform_config.h"
#include "SimpleC64.h"
#include "c64/c64.h"
#include "ReSidC64Sid.h"

repeating_timer reapCommandTimer{};
queue_t txQueue;
uint8_t playPauseCommand = PLAY_PAUSE_COMMAND_CODE;
uint8_t volume = INITIAL_VOLUME;
float volumeFactor;
static sid_info sidInfo{};
short visualizationBuffer[FFT_SAMPLES];
bool firstBuffer = true;
volatile bool playPauseQueued = false;
bool rendering = false;
bool loadingSuccessful = true;
CatalogEntry *lastCatalogEntry = {};
auto c64Instance = new c64();
auto residInstance = new ReSidC64Sid();
std::vector<uint8_t> psid_driver;
uint8_t *reloc_driver;
int reloc_size;

const uint8_t PSID_DRIVER[] =
{
#  include "psiddrv.bin"
};

uint_least16_t m_driverAddr;
uint_least16_t m_driverLength;
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
    c64Instance->setBaseSid(residInstance);
    c64Instance->reset();
}

void SIDPlayer::togglePlayPause() {
    queue_add_blocking(&txQueue, &playPauseCommand);
}

void SIDPlayer::ampOn() {
    gpio_pull_up(AMP_CONTROL_PIN);
}

void SIDPlayer::ampOff() {
    if (!SimpleC64::getLineLevelOn()) {
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
}

bool SIDPlayer::lineLevelOn() {
    return false;
}

sid_info *SIDPlayer::getSidInfo() {
    return &sidInfo;
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

bool SIDPlayer::drvReloc(uint_least16_t loadAddr, uint_least32_t dataLen,
                         uint_least8_t relocStartPage, uint_least8_t relocPages) {
    const int startlp = loadAddr >> 8;
    const int endlp = (loadAddr + (dataLen - 1)) >> 8;

    // Check for free space in tune
    if (relocStartPage == 0xff)
        relocPages = 0;
        // Check if we need to find the reloc addr
    else if (relocStartPage == 0) {
        relocPages = 0;
        // find area where to dump the driver in.
        // It's only 1 block long, so any free block we can find
        // between $0400 and $d000 will do.
        for (int i = 4; i < 0xd0; i++) {
            if (i >= startlp && i <= endlp)
                continue;

            if (i >= 0xa0 && i <= 0xbf)
                continue;

            relocStartPage = i;
            relocPages = 1;
            break;
        }
    }

    if (relocPages < 1) {
        printf("ERROR: No space to install psid driver in C64 ram\n");
        return false;
    }

    // Place psid driver into ram
    const uint_least16_t relocAddr = relocStartPage << 8;

    psid_driver.assign(std::begin(PSID_DRIVER), std::end(PSID_DRIVER));
    reloc_driver = psid_driver.data();
    reloc_size = psid_driver.size();

    reloc65 relocator(relocAddr - 10);
    if (!relocator.reloc(&reloc_driver, &reloc_size)) {
        printf("ERROR: Failed whilst relocating psid driver\n");
        return false;
    }

    // Adjust size to not include initialisation data.
    reloc_size -= 10;

    m_driverAddr = relocAddr;
    m_driverLength = static_cast<uint_least16_t>(reloc_size);
    // Round length to end of page
    m_driverLength += 0xff;
    m_driverLength &= 0xff00;

    return true;
}

uint8_t SIDPlayer::iomap(uint_least16_t addr) const {
    return 0;
}

void SIDPlayer::readHeader(BYTE *dataBuf, psidHeader &hdr) {
    // Read v1 fields
    hdr.id = endian_big32(&dataBuf[0]);
    hdr.version = endian_big16(&dataBuf[4]);
    hdr.data = endian_big16(&dataBuf[6]);
    hdr.load = endian_big16(&dataBuf[8]);
    hdr.init = endian_big16(&dataBuf[10]);
    hdr.play = endian_big16(&dataBuf[12]);
    hdr.songs = endian_big16(&dataBuf[14]);
    hdr.start = endian_big16(&dataBuf[16]);
    hdr.speed = endian_big32(&dataBuf[18]);

    std::memcpy(hdr.name, &dataBuf[22], PSID_MAXSTRLEN);
    std::memcpy(hdr.author, &dataBuf[54], PSID_MAXSTRLEN);
    std::memcpy(hdr.released, &dataBuf[86], PSID_MAXSTRLEN);

    if (hdr.version >= 2) {

        // Read v2/3/4 fields
        hdr.flags = endian_big16(&dataBuf[118]);
        hdr.relocStartPage = dataBuf[120];
        hdr.relocPages = dataBuf[121];
        hdr.sidChipBase2 = dataBuf[122];
        hdr.sidChipBase3 = dataBuf[123];
    }
}

void SIDPlayer::installDriver(uint_least16_t init_addr, uint_least16_t play_addr) {
    auto mem = &c64Instance->getMemInterface();
    mem->fillRam(0, static_cast<uint8_t>(0), 0x3ff);
    mem->writeMemByte(0x02a6, 1);
    mem->installResetHook(endian_little16(reloc_driver));
    mem->fillRam(0x0314, &reloc_driver[2], 6);
    int pos = m_driverAddr;

    // Install driver to ram
    mem->fillRam(pos, &reloc_driver[10], reloc_size);

    // Set song number
    mem->writeMemByte(pos, 0);
    pos++;

    // Set tunes speed (VIC/CIA)
    mem->writeMemByte(pos, 0);
    pos++;

    // Set init address
    mem->writeMemWord(pos, init_addr);
    pos += 2;

    // Set play address
    mem->writeMemWord(pos, play_addr);
    pos += 2;

    // Set init address io bank value
    mem->writeMemByte(pos, 0);
    pos++;

    // Set play address io bank value
    mem->writeMemByte(pos, 0);
    pos++;

    // Set PAL/NTSC flag
    mem->writeMemByte(pos, 1);
    pos++;

    mem->writeMemByte(pos, 1);
    pos++;

    mem->writeMemByte(pos, 1 << MOS6510::SR_INTERRUPT);
}

volatile bool SIDPlayer::loadPSID(CatalogEntry *sidFile) {
    FIL pFile;
    BYTE header[SID_HEADER_SIZE];
    BYTE buffer[SID_LOAD_BUFFER_SIZE];
    UINT bytesRead;
    if (f_open(&pFile, sidFile->fileName, FA_READ) != FR_OK) return false;
    if (f_read(&pFile, &header, SID_HEADER_SIZE, &bytesRead) != FR_OK) return false;
    if (bytesRead < SID_HEADER_SIZE) return false;
    // TODO: Rewrite, similar to the PSID class in libsidplayfp, so that speed, chip model/count etc are properly set.
    psidHeader hdr{};
    readHeader(header, hdr);

    uint_least16_t powerOnDelay = 0x1FFF;
    for (int i = 0; i <= powerOnDelay; i++) {
        for (int j = 0; j < 100; j++)
            c64Instance->clock();
    }

    auto mem = &c64Instance->getMemInterface();

    f_lseek(&pFile, hdr.data + 2);
    uint16_t offset = hdr.load;
    int totalBytesRead = 0;
    while (true) {
        f_read(&pFile, buffer, SID_LOAD_BUFFER_SIZE, &bytesRead);
        if (bytesRead == 0) break;
        mem->fillRam(offset, buffer, bytesRead);
        offset += SID_LOAD_BUFFER_SIZE;
        totalBytesRead += bytesRead;
    }
    const uint_least16_t start = hdr.load;
    const uint_least16_t end = start + totalBytesRead;

    drvReloc(hdr.load, totalBytesRead, hdr.relocStartPage, hdr.relocPages);
    installDriver(hdr.init, hdr.play);

    mem->writeMemWord(0x2d, end); // Variables start
    mem->writeMemWord(0x2f, end); // Arrays start
    mem->writeMemWord(0x31, end); // Strings start
    mem->writeMemWord(0xac, start);
    mem->writeMemWord(0xae, end);
    f_close(&pFile);

    c64Instance->resetCpu();

    return true;
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void SIDPlayer::tryJSRToPlayAddr() {
    if (!SimpleC64::cpuJSRWithWatchdog(sidInfo.play_addr, 0)) {
        loadingSuccessful = false;
    }
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void SIDPlayer::generateSamples(audio_buffer *buffer) {
    auto *samples = reinterpret_cast<int16_t *>(buffer->buffer->bytes);
    c64Instance->clock();
    residInstance->output(samples, SAMPLES_PER_BUFFER);
    // tryJSRToPlayAddr();
    // if (sidInfo.speed == USE_CIA) {
    //     SimpleC64::sid_synth_render(samples, SAMPLES_PER_BUFFER / 2);
    //     tryJSRToPlayAddr();
    //     SimpleC64::sid_synth_render(samples + SAMPLES_PER_BUFFER / 2, SAMPLES_PER_BUFFER / 2);
    // } else {
    //     SimpleC64::sid_synth_render(samples, SAMPLES_PER_BUFFER);
    // }
    std::copy(samples, samples + (FFT_SAMPLES - SAMPLES_PER_BUFFER),
              visualizationBuffer + (firstBuffer ? 0 : SAMPLES_PER_BUFFER));
    firstBuffer = !firstBuffer;
    for (int16_t i = 0; i < SAMPLES_PER_BUFFER; i++) {
        if (samples[i]) {
            samples[i] = static_cast<int16_t>(static_cast<float>(samples[i]) * volumeFactor);
        }
    }
    buffer->sample_count = buffer->max_sample_count;
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
                    // SimpleC64::sidPoke(24, 15);
                    // SimpleC64::cpuJSR(sidInfo.init_addr, sidInfo.start_song);
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
            generateSamples(buffer);
            give_audio_buffer(audioBufferPool, buffer);
        }
    }
}
