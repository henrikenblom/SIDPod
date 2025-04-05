/*
 * c64.h
 */

#ifndef SID_SID_H_
#define SID_SID_H_

#include <stdint.h>
#include "ff.h"


#define CLOCKFREQ 985248

extern unsigned char memory[];

struct sid_info {
    uint16_t version;
    uint16_t load_addr;
    uint16_t init_addr;
    uint16_t play_addr;
    uint8_t subsongs;
    uint8_t start_song;
    uint8_t speed;
    bool rsid;
    char title[32];
    char author[32];
    char released[32];
};

//----------------------------------------------
class C64 {
public:
    static void synth_init();

    static void sidPoke(int reg, unsigned char val);

    static void cpuReset();

    static void cpuJSR(unsigned short npc, unsigned char na);

    static bool cpuJSRWithWatchdog(unsigned short npc, unsigned char na);

    static void c64Init();

    static void setLineLevel(bool on);

    static bool getLineLevelOn();

    static bool sid_load_from_file(TCHAR file_name[], struct sid_info *info);

    static void sid_synth_render(short *buffer, size_t len);
};

//----------------------------------------------

#endif /* SID_SID_H_ */
