/*
 * c64.h
 */

#ifndef SID_SID_H_
#define SID_SID_H_

#include <stdint.h>
#include <pico/audio.h>

#include "ff.h"
#include "reSID/siddefs.h"

#define CLOCKFREQ 985248
#define IS_BIT_SET(value, bit) (((value) & (1 << (bit))) != 0)

extern unsigned char memory[];

constexpr int PSID_MAXSTRLEN = 32;

constexpr int psid_headerSize = 118;
constexpr int psidv2_headerSize = psid_headerSize + 6;

struct sid_info {
    uint32_t id; // 'PSID' or 'RSID' (ASCII)
    uint16_t version; // 1, 2, 3 or 4
    uint16_t data; // 16-bit offset to binary data in file
    uint16_t load; // 16-bit C64 address to load file to
    uint16_t init; // 16-bit C64 address of init subroutine
    uint16_t play; // 16-bit C64 address of play subroutine
    uint16_t songs; // number of songs
    uint16_t start; // start song out of [1..256]
    uint32_t speed; // 32-bit speed info
    // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[PSID_MAXSTRLEN]; // ASCII strings, 31 characters long and
    char author[PSID_MAXSTRLEN]; // terminated by a trailing zero
    char released[PSID_MAXSTRLEN]; //

    uint16_t flags; // only version >= 2
    uint8_t relocStartPage; // only version >= 2ng
    uint8_t relocPages; // only version >= 2ng
    uint8_t sidChipBase2; // only version >= 3
    uint8_t sidChipBase3; // only version >= 4

    bool originalFileFormat;
};

//----------------------------------------------
class C64 {
public:
    static void synth_init();

    static void sidPoke(int reg, unsigned char val, int8_t sid);

    static reg8 sidPeek(unsigned short reg);

    static void cpuReset();

    static void cpuJSR(unsigned short npc, unsigned char na);

    static bool cpuJSRWithWatchdog(unsigned short npc, unsigned char na);

    static void c64Init();

    static bool useCIA();

    static volatile bool tryJSRToPlayAddr();

    static volatile bool generateSamples(audio_buffer *buffer, float volumeFactor);

    static void setLineLevel(bool on);

    static bool getLineLevelOn();

    static bool sid_load_from_file(TCHAR file_name[]);

    static sid_info *getSidInfo();

    static void print_sid_info();

    static void readHeader(BYTE *buffer, sid_info &info);

    static void sid_synth_render(short *buffer, size_t len);
};

//----------------------------------------------

#endif /* SID_SID_H_ */
