//
// Created by Henrik Enblom on 2025-04-15.
//
#include "TuneInfo.h"

#include <cstdio>
#include <cstring>
#include "Sidendian.h"
#include "platform_config.h"

TuneInfo::TuneInfo(unsigned char *buffer, size_t size, FIL pFile) {
    readHeader(buffer);

    compatibility = COMPATIBILITY_C64;
    if (id == PSID_ID && version == 1) {
        compatibility = COMPATIBILITY_PSID;
    } else if (id == RSID_ID) {
        compatibility = COMPATIBILITY_R64;
    }
    if (version >= 2) {
        if (flags & PSID_SPECIFIC)
            compatibility = COMPATIBILITY_PSID;
        else if (flags & PSID_BASIC)
            compatibility = COMPATIBILITY_BASIC;

        clock = CLOCK_UNKNOWN;
        switch (flags & PSID_CLOCK) {
            case PSID_CLOCK_ANY:
                clock = CLOCK_ANY;
                break;
            case PSID_CLOCK_PAL:
                clock = CLOCK_PAL;
                break;
            case PSID_CLOCK_NTSC:
                clock = CLOCK_NTSC;
                break;
            default:
                break;
        }
        m_sidModels[0] = getSidModel(flags >> 4);

        if (version >= 3) {
            if (validateAddress(sidChipBase2)) {
                m_sidChipAddresses.push_back(0xd000 | (sidChipBase2 << 4));

                m_sidModels.push_back(getSidModel(flags >> 6));
            }

            if (version >= 4) {
                if (sidChipBase3 != sidChipBase2
                    && validateAddress(sidChipBase3)) {
                    m_sidChipAddresses.push_back(0xd000 | (sidChipBase3 << 4));

                    m_sidModels.push_back(getSidModel(flags >> 8));
                }
            }
        }
    }

    if (compatibility == COMPATIBILITY_R64) {
        // Real C64 tunes appear as CIA
        speed = ~0;
    }

    convertOldStyleSpeedToTables(speed, clock);

    if (songs > MAX_SONGS) {
        songs = MAX_SONGS;
    } else if (songs == 0) {
        songs = 1;
    }
    if (start == 0 || start > songs) {
        start = 1;
    }

    currentSong = start;

    dataFileLen = size;
    c64dataLen = size - fileOffset;

    UINT bytesRead;
    BYTE addrBuffer[2];
    f_lseek(&pFile, fileOffset);
    f_read(&pFile, buffer, 2, &bytesRead);
    resolveAddrs(endian_16(*(addrBuffer + 1), *addrBuffer));
    checkRelocInfo();

    if (dataFileLen >= 2) {
        fixLoad = endian_little16(addrBuffer) == load + 2;
    }
}

bool TuneInfo::checkRelocInfo() {
    // Fix relocation information
    if (relocStartPage == 0xFF) {
        relocPages = 0;
        return true;
    }
    if (relocPages == 0) {
        relocStartPage = 0;
        return true;
    }

    // Calculate start/end page
    const uint_least8_t startp = relocStartPage;
    const uint_least8_t endp = (startp + relocPages - 1) & 0xff;
    if (endp < startp) {
        return false;
    } {
        // Check against load range
        const uint_least8_t startlp = (uint_least8_t) (load >> 8);
        const uint_least8_t endlp = startlp + (uint_least8_t) (c64dataLen - 1 >> 8);

        if (((startp <= startlp) && (endp >= startlp))
            || ((startp <= endlp) && (endp >= endlp))) {
            return false;
        }
    }

    // Check that the relocation information does not use the following
    // memory areas: 0x0000-0x03FF, 0xA000-0xBFFF and 0xD000-0xFFFF
    if ((startp < 0x04)
        || ((0xa0 <= startp) && (startp <= 0xbf))
        || (startp >= 0xd0)
        || ((0xa0 <= endp) && (endp <= 0xbf))
        || (endp >= 0xd0)) {
        return false;
    }

    return true;
}

bool TuneInfo::resolveAddrs(uint_least16_t loadCandidate) {
    // Originally used as a first attempt at an RSID
    // style format. Now reserved for future use
    if (play == 0xffff) {
        play = 0;
    }

    // loadAddr = 0 means, the address is stored in front of the C64 data.
    if (load == 0) {
        if (c64dataLen < 2) {
            printf("Error: Corrupt file\n");
            return false;
        }

        load = loadCandidate;
        fileOffset += 2;
        c64dataLen -= 2;
    }

    if (compatibility == COMPATIBILITY_BASIC) {
        if (init != 0) {
            printf("Error: Corrupt file\n");
            return false;
        }
    } else if (init == 0) {
        init = load;
    }
    return true;
}

void TuneInfo::convertOldStyleSpeedToTables(uint_least32_t speed, clock_t clock) {
    // Create the speed/clock setting tables.
    //
    // This routine implements the PSIDv2NG compliant speed conversion. All tunes
    // above 32 use the same song speed as tune 32
    const unsigned int toDo = std::min(songs, MAX_SONGS);
    for (unsigned int s = 0; s < toDo; s++) {
        clockSpeed[s] = clock;
        songSpeed[s] = (speed & 1) ? SPEED_CIA_1A : SPEED_VBI;

        if (s < 31) {
            speed >>= 1;
        }
    }
}

/**
 * Check if extra SID addres is valid for PSID specs.
 */
bool TuneInfo::validateAddress(uint_least8_t address) {
    // Only even values are valid.
    if (address & 1)
        return false;

    // Ranges $00-$41 ($D000-$D410) and $80-$DF ($D800-$DDF0) are invalid.
    // Any invalid value means that no second SID is used, like $00.
    if (address <= 0x41
        || (address >= 0x80 && address <= 0xdf))
        return false;

    return true;
}

TuneInfo::model_t TuneInfo::getSidModel(uint_least16_t modelFlag) {
    if ((modelFlag & PSID_SIDMODEL_ANY) == PSID_SIDMODEL_ANY)
        return SIDMODEL_ANY;

    if (modelFlag & PSID_SIDMODEL_6581)
        return SIDMODEL_6581;

    if (modelFlag & PSID_SIDMODEL_8580)
        return SIDMODEL_8580;

    return SIDMODEL_UNKNOWN;
}

void TuneInfo::readHeader(unsigned char *header) {
    // Read v1 fields
    id = endian_big32(&header[0]);
    version = endian_big16(&header[4]);
    data = endian_big16(&header[6]);
    load = endian_big16(&header[8]);
    init = endian_big16(&header[10]);
    play = endian_big16(&header[12]);
    songs = endian_big16(&header[14]);
    start = endian_big16(&header[16]) - 1;
    speed = endian_big32(&header[18]);

    std::memcpy(name, &header[22], PSID_MAXSTRLEN);
    std::memcpy(author, &header[54], PSID_MAXSTRLEN);
    std::memcpy(released, &header[86], PSID_MAXSTRLEN);

    if (version >= 2) {
        // Read v2/3/4 fields
        flags = endian_big16(&header[118]);
        relocStartPage = header[120];
        relocPages = header[121];
        if (version >= 3) {
            sidChipBase2 = header[122];
        }
        if (version >= 4) {
            sidChipBase3 = header[123];
        }
    }

    // TODO: Adapt to the method used in libsidplayfp
    if (!load) {
        load = header[data];
        load |= header[data + 1] << 8;
        originalFileFormat = true;
    } else {
        originalFileFormat = false;
    }
    if (!init) {
        init = load;
    }
}
