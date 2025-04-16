//
// Created by Henrik Enblom on 2025-04-15.
//

#ifndef TUNEINFO_H
#define TUNEINFO_H
#include <cstdint>
#include <ff.h>
#include <vector>

#define IS_BIT_SET(value, bit) (((value) & (1 << (bit))) != 0)
constexpr int PSID_MAXSTRLEN = 32;
static constexpr uint16_t MAX_SONGS = 256;

class TuneInfo {
public:

    typedef enum {
        CLOCK_UNKNOWN,
        CLOCK_PAL,
        CLOCK_NTSC,
        CLOCK_ANY
    } clock_t;

    typedef enum {
        SIDMODEL_UNKNOWN,
        SIDMODEL_6581,
        SIDMODEL_8580,
        SIDMODEL_ANY
    } model_t;

    typedef enum {
        COMPATIBILITY_C64, ///< File is C64 compatible
        COMPATIBILITY_PSID, ///< File is PSID specific
        COMPATIBILITY_R64, ///< File is Real C64 only
        COMPATIBILITY_BASIC ///< File requires C64 Basic
    } compatibility_t;

    TuneInfo(unsigned char *buffer, size_t size, FIL pFile);

    bool checkRelocInfo();

    bool resolveAddrs(uint_least16_t loadCandidate);

    void convertOldStyleSpeedToTables(uint_least32_t speed, clock_t clock);

    bool validateAddress(uint_least8_t address);

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

    uint_least16_t m_driverAddr = 0;
    uint_least16_t m_driverLength = 0;

    uint_least16_t m_powerOnDelay = 0;

    uint8_t currentSong;

    std::vector<model_t> m_sidModels;
    std::vector<uint_least16_t> m_sidChipAddresses;

    uint_least32_t dataFileLen;
    uint_least32_t fileOffset;
    uint_least32_t c64dataLen;
    bool fixLoad;

    compatibility_t compatibility;
    uint_least8_t songSpeed[MAX_SONGS];
    clock_t clockSpeed[MAX_SONGS];
    clock_t clock = CLOCK_UNKNOWN;

    bool originalFileFormat;

    static const int SPEED_VBI = 0;

    /// CIA 1 Timer A
    static const int SPEED_CIA_1A = 60;

protected:


    void readHeader(unsigned char *buffer);

    model_t getSidModel(uint_least16_t modelFlag);

    enum {
        PSID_MUS = 1 << 0,
        PSID_SPECIFIC = 1 << 1, // These two are mutally exclusive
        PSID_BASIC = 1 << 1,
        PSID_CLOCK = 3 << 2,
        PSID_SIDMODEL = 3 << 4
    };

    enum {
        PSID_CLOCK_UNKNOWN = 0,
        PSID_CLOCK_PAL = 1 << 2,
        PSID_CLOCK_NTSC = 1 << 3,
        PSID_CLOCK_ANY = PSID_CLOCK_PAL | PSID_CLOCK_NTSC
    };

    enum {
        PSID_SIDMODEL_UNKNOWN = 0,
        PSID_SIDMODEL_6581 = 1,
        PSID_SIDMODEL_8580 = 2,
        PSID_SIDMODEL_ANY = PSID_SIDMODEL_6581 | PSID_SIDMODEL_8580
    };
};
#endif //TUNEINFO_H
