/*
 * sid.h
 *
 *  Created on: 14 lip 2016
 *      Author: Korzo
 */

#ifndef SID_SID_H_
#define SID_SID_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ff.h"

//----------------------------------------------

void synth_init(unsigned long mixfrq);

void sidPoke(int reg, unsigned char val);

void cpuReset(void);

void cpuJSR(unsigned short npc, unsigned char na);

bool cpuJSRWithWatchdog(unsigned short npc, unsigned char na);

void c64Init(int nSampleRate);

void setLineLevel(bool on);

bool getLineLevelOn();

struct sid_info {
    uint16_t version;
    uint16_t load_addr;
    uint16_t init_addr;
    uint16_t play_addr;
    uint8_t subsongs;
    uint8_t start_song;
    uint8_t speed;
    char title[32];
    char author[32];
    char released[32];
};

bool sid_load_from_file(TCHAR file_name[], struct sid_info *info);

void sid_synth_render(uint16_t *buffer, size_t len);

extern unsigned char memory[];

//----------------------------------------------

#endif /* SID_SID_H_ */
