#ifndef SIDPOD_SIDPLAYER_H
#define SIDPOD_SIDPLAYER_H

#include <c64/c64.h>
#include <pico/audio.h>

#include "SimpleC64.h"
#include "../PSIDCatalog.h"

#define PLAY_PAUSE_COMMAND_CODE     123

extern short visualizationBuffer[];

constexpr int PSID_MAXSTRLEN = 32;

constexpr int psid_headerSize = 118;
constexpr int psidv2_headerSize = psid_headerSize + 6;

using buffer_t = BYTE[];

struct psidHeader           // all values are big-endian
{
    uint32_t id;                   // 'PSID' or 'RSID' (ASCII)
    uint16_t version;              // 1, 2, 3 or 4
    uint16_t data;                 // 16-bit offset to binary data in file
    uint16_t load;                 // 16-bit C64 address to load file to
    uint16_t init;                 // 16-bit C64 address of init subroutine
    uint16_t play;                 // 16-bit C64 address of play subroutine
    uint16_t songs;                // number of songs
    uint16_t start;                // start song out of [1..256]
    uint32_t speed;                // 32-bit speed info
    // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[PSID_MAXSTRLEN];     // ASCII strings, 31 characters long and
    char author[PSID_MAXSTRLEN];   // terminated by a trailing zero
    char released[PSID_MAXSTRLEN]; //

    uint16_t flags;                // only version >= 2
    uint8_t relocStartPage;        // only version >= 2ng
    uint8_t relocPages;            // only version >= 2ng
    uint8_t sidChipBase2;          // only version >= 3
    uint8_t sidChipBase3;          // only version >= 4
};

class SIDPlayer {

public:
    static void initAudio();

    static volatile bool loadPSID(CatalogEntry *sidFile);

    static CatalogEntry *getCurrentlyLoaded();

    static void togglePlayPause();

    static void ampOn();

    static void ampOff();

    static void volumeUp();

    static void volumeDown();

    static uint8_t getVolume();

    static bool isPlaying();

    static void toggleLineLevel();

    static bool lineLevelOn();

    static sid_info *getSidInfo();

    static bool loadingWasSuccessful();

    static void resetState();

private:
    static volatile void tryJSRToPlayAddr();

    static void updateVolumeFactor();

    static volatile void generateSamples(audio_buffer *buffer);

    [[noreturn]] static void core1Main();

    static volatile bool reapCommand(struct repeating_timer *t);

    static bool drvReloc(uint_least16_t loadAddr, uint_least32_t dataLen, uint_least8_t relocStartPage, uint_least8_t relocPages);

    uint8_t iomap(uint_least16_t addr) const;

    static void readHeader(BYTE *dataBuf, psidHeader &hdr);

    static void installDriver(uint_least16_t init_addr, uint_least16_t play_addr);
};

#endif //SIDPOD_SIDPLAYER_H
