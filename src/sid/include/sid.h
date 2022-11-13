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

//----------------------------------------------

void synth_init(unsigned long mixfrq);

void sidPoke(int reg, unsigned char val);

void cpuReset(void);

void cpuResetTo(unsigned short npc, unsigned char na);

void cpuJSR(unsigned short npc, unsigned char na);

void c64Init(int nSampleRate);

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

bool sid_load_from_memory(void *data, size_t size, struct sid_info *info);

void set_volume(int8_t new_volume);

void sid_synth_render(uint16_t *buffer, size_t len);

extern unsigned char memory[];

//----------------------------------------------

#endif /* SID_SID_H_ */
