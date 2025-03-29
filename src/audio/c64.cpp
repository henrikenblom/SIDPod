/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * SID Codec for rockbox based on the TinySID engine
 * 
 * Written by Tammo Hinrichs (kb) and Rainer Sinsch in 1998-1999
 * Ported to rockbox on 14 April 2006
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*****************************
* kb explicitly points out that this emulation sounds crappy, though
* we decided to put it open source so everyone can enjoy sidmusic
* on rockbox
*
*****************************/

/*********************
* v1.1
* Added 16-04-2006: Rainer Sinsch
* Removed all time critical floating point operations and
* replaced them with quick & dirty integer calculations
*
* Added 17-04-2006: Rainer Sinsch
* Improved quick & dirty integer calculations for the resonant filter
* Improved audio quality by 4 bits
*
* v1.2
* Added 17-04-2006: Dave Chapman
* Improved file loading
*
* Added 17-04-2006: Rainer Sinsch
* Added sample routines
* Added cia timing routines
* Added fast forwarding capabilities
* Corrected bug in sid loading
*
* v1.2.1
* Added 04-05-2006: Rainer Sinsch
* Implemented Marco Alanens suggestion for subsong selection:
* Select the subsong by seeking: Each second represents a subsong
*
**************************/

#include "c64.h"

#include <cmath>
#include <cstdio>
#include <string.h>
#include "../platform_config.h"
#include "reSID/sid.h"

#define ICODE_ATTR
#define IDATA_ATTR
#define ICONST_ATTR


void sidPoke(int reg, unsigned char val) ICODE_ATTR;

#define FLAG_N 128
#define FLAG_V 64
#define FLAG_D 8
#define FLAG_I 4
#define FLAG_Z 2
#define FLAG_C 1

#define imp 0
#define imm 1
#define _abs 2
#define absx 3
#define absy 4
#define zp 6
#define zpx 7
#define zpy 8
#define ind 9
#define indx 10
#define indy 11
#define acc 12
#define rel 13

enum {
    adc, _and, asl, bcc, bcs, beq, bit, bmi, bne, bpl, _brk, bvc, bvs, clc,
    cld, cli, clv, cmp, cpx, cpy, dec, dex, dey, eor, inc, inx, iny, jmp,
    jsr, lda, ldx, ldy, lsr, _nop, ora, pha, php, pla, plp, rol, ror, rti,
    rts, sbc, sec, sed, sei, sta, stx, sty, tax, tay, tsx, txa, txs, tya,
    xxx
};

/* SID register definition */
struct s6581 {
    struct sidvoice {
        unsigned short freq;
        unsigned short pulse;
        unsigned char wave;
        unsigned char ad;
        unsigned char sr;
    } v[3];

    unsigned char ffreqlo;
    unsigned char ffreqhi;
    unsigned char res_ftv;
    unsigned char ftp_vol;
};

/* internal oscillator def */
struct sidosc {
    unsigned long freq;
    unsigned long pulse;
    unsigned char wave;
    unsigned char filter;
    unsigned long attack;
    unsigned long decay;
    unsigned long sustain;
    unsigned long release;
    unsigned long counter;
    signed long envval;
    unsigned char envphase;
    unsigned long noisepos;
    unsigned long noiseval;
    unsigned char noiseout;
};

/* internal filter def */
struct sidflt {
    int freq;
    unsigned char l_ena;
    unsigned char b_ena;
    unsigned char h_ena;
    unsigned char v3ena;
    int vol;
    int rez;
    int h;
    int b;
    int l;
};

/* ------------------------ pseudo-constants (depending on mixing freq) */
static int mixing_frequency IDATA_ATTR;
static unsigned long freqmul IDATA_ATTR;
static int filtmul IDATA_ATTR;
#ifndef ROCKBOX
unsigned long attacks[16] IDATA_ATTR;
unsigned long releases[16] IDATA_ATTR;
#endif

/* ------------------------------------------------------------ globals */
static struct s6581 sid IDATA_ATTR;
static struct sidosc osc[3] IDATA_ATTR;
static struct sidflt filter IDATA_ATTR;

static long watchdog_counter = 0;

/* ------------------------------------------------------ C64 Emu Stuff */
static unsigned char bval IDATA_ATTR;
static unsigned short wval IDATA_ATTR;
/* -------------------------------------------------- Register & memory */
static unsigned char a, x, y, s, p IDATA_ATTR;
static unsigned short pc IDATA_ATTR;

unsigned char memory[65536];

/* ----------------------------------------- Variables for sample stuff */
static int sample_active IDATA_ATTR;
static int sample_position, sample_start, sample_end, sample_repeat_start IDATA_ATTR;
static int fracPos IDATA_ATTR; /* Fractal position of sample */
static int sample_period IDATA_ATTR;
static int sample_repeats IDATA_ATTR;
static int sample_order IDATA_ATTR;
static int sample_nibble IDATA_ATTR;

static int internal_period, internal_order, internal_start, internal_end,
        internal_add, internal_repeat_times, internal_repeat_start IDATA_ATTR;

int8_t map_shift_bits = 2;

/* ---------------------------------------------------------- constants */
static const float attackTimes[16] ICONST_ATTR =
{
    0.0022528606f, 0.0080099577f, 0.0157696042f, 0.0237795619f,
    0.0372963655f, 0.0550684591f, 0.0668330845f, 0.0783473987f,
    0.0981219818f, 0.244554021f, 0.489108042f, 0.782472742f,
    0.977715461f, 2.93364701f, 4.88907793f, 7.82272493f
};
static const float decayReleaseTimes[16] ICONST_ATTR =
{
    0.00891777693f, 0.024594051f, 0.0484185907f, 0.0730116639f, 0.114512475f,
    0.169078356f, 0.205199432f, 0.240551975f, 0.301266125f, 0.750858245f,
    1.50171551f, 2.40243682f, 3.00189298f, 9.00721405f, 15.010998f, 24.0182111f
};

static const int opcodes[256] ICONST_ATTR = {
    _brk, ora, xxx, xxx, xxx, ora, asl, xxx, php, ora, asl, xxx, xxx, ora, asl, xxx,
    bpl, ora, xxx, xxx, xxx, ora, asl, xxx, clc, ora, xxx, xxx, xxx, ora, asl, xxx,
    jsr, _and, xxx, xxx, bit, _and, rol, xxx, plp, _and, rol, xxx, bit, _and, rol, xxx,
    bmi, _and, xxx, xxx, xxx, _and, rol, xxx, sec, _and, xxx, xxx, xxx, _and, rol, xxx,
    rti, eor, xxx, xxx, xxx, eor, lsr, xxx, pha, eor, lsr, xxx, jmp, eor, lsr, xxx,
    bvc, eor, xxx, xxx, xxx, eor, lsr, xxx, cli, eor, xxx, xxx, xxx, eor, lsr, xxx,
    rts, adc, xxx, xxx, xxx, adc, ror, xxx, pla, adc, ror, xxx, jmp, adc, ror, xxx,
    bvs, adc, xxx, xxx, xxx, adc, ror, xxx, sei, adc, xxx, xxx, xxx, adc, ror, xxx,
    xxx, sta, xxx, xxx, sty, sta, stx, xxx, dey, xxx, txa, xxx, sty, sta, stx, xxx,
    bcc, sta, xxx, xxx, sty, sta, stx, xxx, tya, sta, txs, xxx, xxx, sta, xxx, xxx,
    ldy, lda, ldx, xxx, ldy, lda, ldx, xxx, tay, lda, tax, xxx, ldy, lda, ldx, xxx,
    bcs, lda, xxx, xxx, ldy, lda, ldx, xxx, clv, lda, tsx, xxx, ldy, lda, ldx, xxx,
    cpy, cmp, xxx, xxx, cpy, cmp, dec, xxx, iny, cmp, dex, xxx, cpy, cmp, dec, xxx,
    bne, cmp, xxx, xxx, xxx, cmp, dec, xxx, cld, cmp, xxx, xxx, xxx, cmp, dec, xxx,
    cpx, sbc, xxx, xxx, cpx, sbc, inc, xxx, inx, sbc, _nop, xxx, cpx, sbc, inc, xxx,
    beq, sbc, xxx, xxx, xxx, sbc, inc, xxx, sed, sbc, xxx, xxx, xxx, sbc, inc, xxx
};


static const int modes[256] ICONST_ATTR = {
    imp, indx, xxx, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, _abs, _abs, _abs, xxx,
    rel, indy, xxx, xxx, xxx, zpx, zpx, xxx, imp, absy, xxx, xxx, xxx, absx, absx, xxx,
    _abs, indx, xxx, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, _abs, _abs, _abs, xxx,
    rel, indy, xxx, xxx, xxx, zpx, zpx, xxx, imp, absy, xxx, xxx, xxx, absx, absx, xxx,
    imp, indx, xxx, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, _abs, _abs, _abs, xxx,
    rel, indy, xxx, xxx, xxx, zpx, zpx, xxx, imp, absy, xxx, xxx, xxx, absx, absx, xxx,
    imp, indx, xxx, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, ind, _abs, _abs, xxx,
    rel, indy, xxx, xxx, xxx, zpx, zpx, xxx, imp, absy, xxx, xxx, xxx, absx, absx, xxx,
    imm, indx, xxx, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, _abs, _abs, _abs, xxx,
    rel, indy, xxx, xxx, zpx, zpx, zpy, xxx, imp, absy, acc, xxx, xxx, absx, absx, xxx,
    imm, indx, imm, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, _abs, _abs, _abs, xxx,
    rel, indy, xxx, xxx, zpx, zpx, zpy, xxx, imp, absy, acc, xxx, absx, absx, absy, xxx,
    imm, indx, xxx, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, _abs, _abs, _abs, xxx,
    rel, indy, xxx, xxx, zpx, zpx, zpx, xxx, imp, absy, acc, xxx, xxx, absx, absx, xxx,
    imm, indx, xxx, xxx, zp, zp, zp, xxx, imp, imm, acc, xxx, _abs, _abs, _abs, xxx,
    rel, indy, xxx, xxx, zpx, zpx, zpx, xxx, imp, absy, acc, xxx, xxx, absx, absx, xxx
};

SID reSID;
cycle_count csdelta;
int counter = 0;

/* Routines for quick & dirty float calculation */

static inline int quickfloat_ConvertFromInt(int i) {
    return (i << 16);
}

static inline int quickfloat_ConvertFromFloat(float f) {
    return (int) (f * (1 << 16));
}

static inline int quickfloat_Multiply(int a, int b) {
    return (a >> 8) * (b >> 8);
}

static inline int quickfloat_ConvertToInt(int i) {
    return (i >> 16);
}

/* Get the bit from an unsigned long at a specified position */
static inline unsigned char get_bit(unsigned long val, unsigned char b) {
    return (unsigned char) ((val >> b) & 1);
}


static inline int GenerateDigi(int sIn) {
    static int sample = 0;

    if (!sample_active) return (sIn);

    if ((sample_position < sample_end) && (sample_position >= sample_start)) {
        sIn += sample;

        fracPos += 985248 / sample_period;

        if (fracPos > mixing_frequency) {
            fracPos %= mixing_frequency;

            if (sample_order == 0) {
                sample_nibble++;
                if (sample_nibble == 2) {
                    sample_nibble = 0;
                    sample_position++;
                }
            } else {
                sample_nibble--;
                if (sample_nibble < 0) {
                    sample_nibble = 1;
                    sample_position++;
                }
            }
            if (sample_repeats) {
                if (sample_position > sample_end) {
                    sample_repeats--;
                    sample_position = sample_repeat_start;
                } else sample_active = 0;
            }

            sample = memory[sample_position & 0xffff];
            if (sample_nibble == 1)
                sample = (sample & 0xf0) >> 4;
            else sample = sample & 0x0f;

            sample -= 7;
            sample <<= 10;
        }
    }

    return (sIn);
}

/* ------------------------------------------------------------- synthesis
   initialize SID and frequency dependant values */
void C64::synth_init(unsigned long mixfrq) {
    reSID.reset();
    reSID.set_sampling_parameters(CLOCKFREQ, SAMPLE_FAST, SAMPLE_RATE);
    csdelta = round((float) CLOCKFREQ / ((float) SAMPLE_RATE / SAMPLES_PER_BUFFER));
    int i;
    mixing_frequency = mixfrq;
    fracPos = 0;
    freqmul = 15872000 / mixfrq;
    filtmul = quickfloat_ConvertFromFloat(21.5332031f) / mixfrq;
    for (i = 0; i < 16; i++) {
        attacks[i] = (int) (0x1000000 / (attackTimes[i] * mixfrq));
        releases[i] = (int) (0x1000000 / (decayReleaseTimes[i] * mixfrq));
    }
    memset(&sid, 0, sizeof(sid));
    memset(osc, 0, sizeof(osc));
    memset(&filter, 0, sizeof(filter));
    osc[0].noiseval = 0xffffff;
    osc[1].noiseval = 0xffffff;
    osc[2].noiseval = 0xffffff;
}

static inline uint16_t map_sample(int32_t x) {
    return (x + 32768) >> map_shift_bits;
}

void C64::sid_synth_render(short *buffer, size_t len) {
    cycle_count delta_t = round((float) CLOCKFREQ / ((float) SAMPLE_RATE / len));
    reSID.clock(delta_t, buffer, len);
}

/*
* C64 Mem Routines
*/
static inline unsigned char getmem(unsigned short addr) {
    return memory[addr];
}

static inline void setmem(unsigned short addr, unsigned char value) {
    if ((addr & 0xfc00) == 0xd400) {
        C64::sidPoke(addr & 0x1f, value);
        /* New SID-Register */
        if (addr > 0xd418) {
            switch (addr) {
                case 0xd41f: /* Start-Hi */
                    internal_start = (internal_start & 0x00ff) | (value << 8);
                    break;
                case 0xd41e: /* Start-Lo */
                    internal_start = (internal_start & 0xff00) | (value);
                    break;
                case 0xd47f: /* Repeat-Hi */
                    internal_repeat_start = (internal_repeat_start & 0x00ff) | (value << 8);
                    break;
                case 0xd47e: /* Repeat-Lo */
                    internal_repeat_start = (internal_repeat_start & 0xff00) | (value);
                    break;
                case 0xd43e: /* End-Hi */
                    internal_end = (internal_end & 0x00ff) | (value << 8);
                    break;
                case 0xd43d: /* End-Lo */
                    internal_end = (internal_end & 0xff00) | (value);
                    break;
                case 0xd43f: /* Loop-Size */
                    internal_repeat_times = value;
                    break;
                case 0xd45e: /* Period-Hi */
                    internal_period = (internal_period & 0x00ff) | (value << 8);
                    break;
                case 0xd45d: /* Period-Lo */
                    internal_period = (internal_period & 0xff00) | (value);
                    break;
                case 0xd47d: /* Sample Order */
                    internal_order = value;
                    break;
                case 0xd45f: /* Sample Add */
                    internal_add = value;
                    break;
                case 0xd41d: /* Start sampling */
                    sample_repeats = internal_repeat_times;
                    sample_position = internal_start;
                    sample_start = internal_start;
                    sample_end = internal_end;
                    sample_repeat_start = internal_repeat_start;
                    sample_period = internal_period;
                    sample_order = internal_order;
                    switch (value) {
                        case 0xfd:
                            sample_active = 0;
                            break;
                        case 0xfe:
                        case 0xff:
                            sample_active = 1;
                            break;
                        default:
                            return;
                    }
                    break;
            }
        }
    } else memory[addr] = value;
}

/*
* Poke a value into the sid register
*/
void C64::sidPoke(int reg, unsigned char val) {
    reSID.write(reg, val);
    int voice = 0;

    if ((reg >= 7) && (reg <= 13)) {
        voice = 1;
        reg -= 7;
    } else if ((reg >= 14) && (reg <= 20)) {
        voice = 2;
        reg -= 14;
    }

    switch (reg) {
        case 0: {
            /* Set frequency: Low byte */
            sid.v[voice].freq = (sid.v[voice].freq & 0xff00) + val;
            break;
        }
        case 1: {
            /* Set frequency: High byte */
            sid.v[voice].freq = (sid.v[voice].freq & 0xff) + (val << 8);
            break;
        }
        case 2: {
            /* Set pulse width: Low byte */
            sid.v[voice].pulse = (sid.v[voice].pulse & 0xff00) + val;
            break;
        }
        case 3: {
            /* Set pulse width: High byte */
            sid.v[voice].pulse = (sid.v[voice].pulse & 0xff) + (val << 8);
            break;
        }
        case 4: {
            sid.v[voice].wave = val;
            /* Directly look at GATE-Bit!
             * a change may happen twice or more often during one cpujsr
             * Put the Envelope Generator into attack or release phase if desired
            */
            if ((val & 0x01) == 0) osc[voice].envphase = 3;
            else if (osc[voice].envphase == 3) osc[voice].envphase = 0;
            break;
        }

        case 5: {
            sid.v[voice].ad = val;
            break;
        }
        case 6: {
            sid.v[voice].sr = val;
            break;
        }

        case 21: {
            sid.ffreqlo = val;
            break;
        }
        case 22: {
            sid.ffreqhi = val;
            break;
        }
        case 23: {
            sid.res_ftv = val;
            break;
        }
        case 24: {
            sid.ftp_vol = val;
            break;
        }
    }
    return;
}

static inline unsigned char getaddr(int mode) {
    unsigned short ad, ad2;
    switch (mode) {
        case imp:
            return 0;
        case imm:
            return getmem(pc++);
        case _abs:
            ad = getmem(pc++);
            ad |= 256 * getmem(pc++);
            return getmem(ad);
        case absx:
            ad = getmem(pc++);
            ad |= 256 * getmem(pc++);
            ad2 = ad + x;
            return getmem(ad2);
        case absy:
            ad = getmem(pc++);
            ad |= 256 * getmem(pc++);
            ad2 = ad + y;
            return getmem(ad2);
        case zp:
            ad = getmem(pc++);
            return getmem(ad);
        case zpx:
            ad = getmem(pc++);
            ad += x;
            return getmem(ad & 0xff);
        case zpy:
            ad = getmem(pc++);
            ad += y;
            return getmem(ad & 0xff);
        case indx:
            ad = getmem(pc++);
            ad += x;
            ad2 = getmem(ad & 0xff);
            ad++;
            ad2 |= getmem(ad & 0xff) << 8;
            return getmem(ad2);
        case indy:
            ad = getmem(pc++);
            ad2 = getmem(ad);
            ad2 |= getmem((ad + 1) & 0xff) << 8;
            ad = ad2 + y;
            return getmem(ad);
        case acc:
            return a;
    }
    return 0;
}

static inline void setaddr(int mode, unsigned char val) {
    unsigned short ad, ad2;
    switch (mode) {
        case _abs:
            ad = getmem(pc - 2);
            ad |= 256 * getmem(pc - 1);
            setmem(ad, val);
            return;
        case absx:
            ad = getmem(pc - 2);
            ad |= 256 * getmem(pc - 1);
            ad2 = ad + x;
            setmem(ad2, val);
            return;
        case zp:
            ad = getmem(pc - 1);
            setmem(ad, val);
            return;
        case zpx:
            ad = getmem(pc - 1);
            ad += x;
            setmem(ad & 0xff, val);
            return;
        case acc:
            a = val;
            return;
    }
}


static inline void putaddr(int mode, unsigned char val) {
    unsigned short ad, ad2;
    switch (mode) {
        case _abs:
            ad = getmem(pc++);
            ad |= getmem(pc++) << 8;
            setmem(ad, val);
            return;
        case absx:
            ad = getmem(pc++);
            ad |= getmem(pc++) << 8;
            ad2 = ad + x;
            setmem(ad2, val);
            return;
        case absy:
            ad = getmem(pc++);
            ad |= getmem(pc++) << 8;
            ad2 = ad + y;
            setmem(ad2, val);
            return;
        case zp:
            ad = getmem(pc++);
            setmem(ad, val);
            return;
        case zpx:
            ad = getmem(pc++);
            ad += x;
            setmem(ad & 0xff, val);
            return;
        case zpy:
            ad = getmem(pc++);
            ad += y;
            setmem(ad & 0xff, val);
            return;
        case indx:
            ad = getmem(pc++);
            ad += x;
            ad2 = getmem(ad & 0xff);
            ad++;
            ad2 |= getmem(ad & 0xff) << 8;
            setmem(ad2, val);
            return;
        case indy:
            ad = getmem(pc++);
            ad2 = getmem(ad);
            ad2 |= getmem((ad + 1) & 0xff) << 8;
            ad = ad2 + y;
            setmem(ad, val);
            return;
        case acc:
            a = val;
            return;
    }
}


static inline void setflags(int flag, int cond) {
    if (cond) p |= flag;
    else p &= ~flag;
}


static inline void push(unsigned char val) {
    setmem(0x100 + s, val);
    if (s) s--;
}

static inline unsigned char pop(void) {
    if (s < 0xff) s++;
    return getmem(0x100 + s);
}

static inline void branch(int flag) {
    signed char dist;
    dist = (signed char) getaddr(imm);
    wval = pc + dist;
    if (flag) pc = wval;
}

void C64::cpuReset(void) {
    a = x = y = 0;
    p = 0;
    s = 255;
    pc = getaddr(0xfffc);
}

static inline void cpuParse(void) {
    unsigned char opc = getmem(pc++);
    int cmd = opcodes[opc];
    int addr = modes[opc];
    int c;
    switch (cmd) {
        case adc:
            wval = (unsigned short) a + getaddr(addr) + ((p & FLAG_C) ? 1 : 0);
            setflags(FLAG_C, wval & 0x100);
            a = (unsigned char) wval;
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            setflags(FLAG_V, (!!(p & FLAG_C)) ^ (!!(p & FLAG_N)));
            break;
        case _and:
            bval = getaddr(addr);
            a &= bval;
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            break;
        case asl:
            wval = getaddr(addr);
            wval <<= 1;
            setaddr(addr, (unsigned char) wval);
            setflags(FLAG_Z, !wval);
            setflags(FLAG_N, wval & 0x80);
            setflags(FLAG_C, wval & 0x100);
            break;
        case bcc:
            branch(!(p & FLAG_C));
            break;
        case bcs:
            branch(p & FLAG_C);
            break;
        case bne:
            branch(!(p & FLAG_Z));
            break;
        case beq:
            branch(p & FLAG_Z);
            break;
        case bpl:
            branch(!(p & FLAG_N));
            break;
        case bmi:
            branch(p & FLAG_N);
            break;
        case bvc:
            branch(!(p & FLAG_V));
            break;
        case bvs:
            branch(p & FLAG_V);
            break;
        case bit:
            bval = getaddr(addr);
            setflags(FLAG_Z, !(a & bval));
            setflags(FLAG_N, bval & 0x80);
            setflags(FLAG_V, bval & 0x40);
            break;
        case _brk:
            pc = 0; /* Just quit the emulation */
            break;
        case clc:
            setflags(FLAG_C, 0);
            break;
        case cld:
            setflags(FLAG_D, 0);
            break;
        case cli:
            setflags(FLAG_I, 0);
            break;
        case clv:
            setflags(FLAG_V, 0);
            break;
        case cmp:
            bval = getaddr(addr);
            wval = (unsigned short) a - bval;
            setflags(FLAG_Z, !wval);
            setflags(FLAG_N, wval & 0x80);
            setflags(FLAG_C, a >= bval);
            break;
        case cpx:
            bval = getaddr(addr);
            wval = (unsigned short) x - bval;
            setflags(FLAG_Z, !wval);
            setflags(FLAG_N, wval & 0x80);
            setflags(FLAG_C, x >= bval);
            break;
        case cpy:
            bval = getaddr(addr);
            wval = (unsigned short) y - bval;
            setflags(FLAG_Z, !wval);
            setflags(FLAG_N, wval & 0x80);
            setflags(FLAG_C, y >= bval);
            break;
        case dec:
            bval = getaddr(addr);
            bval--;
            setaddr(addr, bval);
            setflags(FLAG_Z, !bval);
            setflags(FLAG_N, bval & 0x80);
            break;
        case dex:
            x--;
            setflags(FLAG_Z, !x);
            setflags(FLAG_N, x & 0x80);
            break;
        case dey:
            y--;
            setflags(FLAG_Z, !y);
            setflags(FLAG_N, y & 0x80);
            break;
        case eor:
            bval = getaddr(addr);
            a ^= bval;
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            break;
        case inc:
            bval = getaddr(addr);
            bval++;
            setaddr(addr, bval);
            setflags(FLAG_Z, !bval);
            setflags(FLAG_N, bval & 0x80);
            break;
        case inx:
            x++;
            setflags(FLAG_Z, !x);
            setflags(FLAG_N, x & 0x80);
            break;
        case iny:
            y++;
            setflags(FLAG_Z, !y);
            setflags(FLAG_N, y & 0x80);
            break;
        case jmp:
            wval = getmem(pc++);
            wval |= 256 * getmem(pc++);
            switch (addr) {
                case _abs:
                    pc = wval;
                    break;
                case ind:
                    pc = getmem(wval);
                    pc |= 256 * getmem(wval + 1);
                    break;
            }
            break;
        case jsr:
            push((pc + 1) >> 8);
            push((pc + 1));
            wval = getmem(pc++);
            wval |= 256 * getmem(pc++);
            pc = wval;
            break;
        case lda:
            a = getaddr(addr);
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            break;
        case ldx:
            x = getaddr(addr);
            setflags(FLAG_Z, !x);
            setflags(FLAG_N, x & 0x80);
            break;
        case ldy:
            y = getaddr(addr);
            setflags(FLAG_Z, !y);
            setflags(FLAG_N, y & 0x80);
            break;
        case lsr:
            bval = getaddr(addr);
            wval = (unsigned char) bval;
            wval >>= 1;
            setaddr(addr, (unsigned char) wval);
            setflags(FLAG_Z, !wval);
            setflags(FLAG_N, wval & 0x80);
            setflags(FLAG_C, bval & 1);
            break;
        case _nop:
            break;
        case ora:
            bval = getaddr(addr);
            a |= bval;
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            break;
        case pha:
            push(a);
            break;
        case php:
            push(p);
            break;
        case pla:
            a = pop();
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            break;
        case plp:
            p = pop();
            break;
        case rol:
            bval = getaddr(addr);
            c = !!(p & FLAG_C);
            setflags(FLAG_C, bval & 0x80);
            bval <<= 1;
            bval |= c;
            setaddr(addr, bval);
            setflags(FLAG_N, bval & 0x80);
            setflags(FLAG_Z, !bval);
            break;
        case ror:
            bval = getaddr(addr);
            c = !!(p & FLAG_C);
            setflags(FLAG_C, bval & 1);
            bval >>= 1;
            bval |= 128 * c;
            setaddr(addr, bval);
            setflags(FLAG_N, bval & 0x80);
            setflags(FLAG_Z, !bval);
            break;
        case rti:
        /* Treat RTI like RTS */
        case rts:
            wval = pop();
            wval |= pop() << 8;
            pc = wval + 1;
            break;
        case sbc:
            bval = getaddr(addr) ^ 0xff;
            wval = (unsigned short) a + bval + ((p & FLAG_C) ? 1 : 0);
            setflags(FLAG_C, wval & 0x100);
            a = (unsigned char) wval;
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a > 127);
            setflags(FLAG_V, (!!(p & FLAG_C)) ^ (!!(p & FLAG_N)));
            break;
        case sec:
            setflags(FLAG_C, 1);
            break;
        case sed:
            setflags(FLAG_D, 1);
            break;
        case sei:
            setflags(FLAG_I, 1);
            break;
        case sta:
            putaddr(addr, a);
            break;
        case stx:
            putaddr(addr, x);
            break;
        case sty:
            putaddr(addr, y);
            break;
        case tax:
            x = a;
            setflags(FLAG_Z, !x);
            setflags(FLAG_N, x & 0x80);
            break;
        case tay:
            y = a;
            setflags(FLAG_Z, !y);
            setflags(FLAG_N, y & 0x80);
            break;
        case tsx:
            x = s;
            setflags(FLAG_Z, !x);
            setflags(FLAG_N, x & 0x80);
            break;
        case txa:
            a = x;
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            break;
        case txs:
            s = x;
            break;
        case tya:
            a = y;
            setflags(FLAG_Z, !a);
            setflags(FLAG_N, a & 0x80);
            break;
    }
}

void C64::cpuJSR(unsigned short npc, unsigned char na) {
    a = na;
    x = 0;
    y = 0;
    p = 0;
    s = 255;
    pc = npc;
    push(0);
    push(0);

    while (pc > 1)
        cpuParse();
}

bool C64::cpuJSRWithWatchdog(unsigned short npc, unsigned char na) {
    watchdog_counter = 0;
    a = na;
    x = 0;
    y = 0;
    p = 0;
    s = 255;
    pc = npc;
    push(0);
    push(0);

    while (pc > 1 && watchdog_counter++ < CPU_JSR_WATCHDOG_ABORT_LIMIT)
        cpuParse();

    return watchdog_counter < CPU_JSR_WATCHDOG_ABORT_LIMIT;
}

void C64::c64Init(int nSampleRate) {
    synth_init(nSampleRate);
    memset(memory, 0, sizeof(memory));
    cpuReset();
}

bool C64::sid_load_from_file(TCHAR file_name[], struct sid_info *info) {
    FIL pFile;
    BYTE header[PSID_HEADER_SIZE];
    BYTE buffer[SID_LOAD_BUFFER_SIZE];
    UINT bytesRead;
    if (f_open(&pFile, file_name, FA_READ) != FR_OK) return false;
    if (f_read(&pFile, &header, PSID_HEADER_SIZE, &bytesRead) != FR_OK) return false;
    if (bytesRead < PSID_HEADER_SIZE) return false;
    unsigned char *pHeader = (unsigned char *) header;
    unsigned char data_file_offset = pHeader[7];

    info->load_addr = pHeader[8] << 8;
    info->load_addr |= pHeader[9];

    info->init_addr = pHeader[10] << 8;
    info->init_addr |= pHeader[11];

    info->play_addr = pHeader[12] << 8;
    info->play_addr |= pHeader[13];

    info->subsongs = pHeader[0xf] - 1;
    info->start_song = pHeader[0x11] - 1;

    info->load_addr = pHeader[data_file_offset];
    info->load_addr |= pHeader[data_file_offset + 1] << 8;

    info->speed = pHeader[0x15];

    strcpy(info->title, (const char *) &pHeader[0x16]);
    strcpy(info->author, (const char *) &pHeader[0x36]);
    strcpy(info->released, (const char *) &pHeader[0x56]);

    f_lseek(&pFile, data_file_offset + 2);
    uint16_t offset = info->load_addr;
    while (true) {
        f_read(&pFile, buffer, SID_LOAD_BUFFER_SIZE, &bytesRead);
        if (bytesRead == 0) break;
        memcpy(&memory[offset], &buffer, bytesRead);
        offset += SID_LOAD_BUFFER_SIZE;
    }
    f_close(&pFile);

    if (info->play_addr == 0) {
        if (!cpuJSRWithWatchdog(info->init_addr, 0)) return false;
        info->play_addr = (memory[0xffff] << 8) | memory[0xfffe];
    }
    return true;
}

void C64::setLineLevel(bool on) {
    if (on) {
        map_shift_bits = 4;
    } else {
        map_shift_bits = 2;
    }
}

bool C64::getLineLevelOn() {
    return map_shift_bits == 4;
}
