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

#include "sid.h"
#include <string.h>
#include "../platform_config.h"
#include "memory.h"

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
        bool muted;
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
static bool is_ntsc = false;
/* -------------------------------------------------- Register & memory */
static unsigned char a, x, y, s, p IDATA_ATTR;
static unsigned short pc IDATA_ATTR;
int32_t cycles = 0;

/* ----------------------------------------- Variables for sample stuff */
static int sample_active IDATA_ATTR;
static int sample_position, sample_start, sample_end, sample_repeat_start IDATA_ATTR;
static int fracPos IDATA_ATTR;  /* Fractal position of sample */
static int sample_period IDATA_ATTR;
static int sample_repeats IDATA_ATTR;
static int sample_order IDATA_ATTR;
static int sample_nibble IDATA_ATTR;

static int internal_period, internal_order, internal_start, internal_end,
        internal_add, internal_repeat_times, internal_repeat_start IDATA_ATTR;

DigiDetectorHandle pDigiDetector;

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

static inline uint32_t sysGetClockRate() {
    if (is_ntsc) {
        return 1022727;    // NTSC system clock (14.31818MHz/14)
    } else {
        return 985249;    // PAL system clock (17.734475MHz/18)
    }
}

static inline int GenerateDigi(int sIn) {
    static int sample = 0;

    if (!sample_active) return (sIn);

    if ((sample_position < sample_end) && (sample_position >= sample_start)) {
        sIn += sample;

        fracPos += sysGetClockRate() / sample_period;

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

            sample = memReadRAM(sample_position & 0xffff);
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
void synth_init(unsigned long mixfrq) {
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
    sid.v[0].muted = false;
    sid.v[1].muted = false;
    sid.v[2].muted = false;
}

void setMuted(uint8_t voice, bool muted) {
    sid.v[voice].muted = muted;
}

unsigned char getAD(uint8_t voice) {
    return sid.v[voice].ad;
}

unsigned char getWave(uint8_t voice) {
    return sid.v[voice].wave;
}

unsigned char getSR(uint8_t voice) {
    return sid.v[voice].sr;
}

short getPulse(uint8_t voice) {
    return sid.v[voice].pulse;
}

short getFreq(uint8_t voice) {
    return sid.v[voice].pulse;
}

int32_t sysCycles() {
    return cycles;
}

static inline uint16_t map_sample(int32_t x) {
    return (x + 32768) >> map_shift_bits;
}

void sid_synth_render(uint16_t *buffer, size_t len) {
    unsigned long bp;
    /* step 1: convert the not easily processable sid registers into some
               more convenient and fast values (makes the thing much faster
              if you process more than 1 sample value at once) */
    unsigned char v;
    for (v = 0; v < 3; v++) {
        osc[v].pulse = (sid.v[v].pulse & 0xfff) << 16;
        osc[v].filter = get_bit(sid.res_ftv, v);
        osc[v].attack = attacks[sid.v[v].ad >> 4];
        osc[v].decay = releases[sid.v[v].ad & 0xf];
        osc[v].sustain = sid.v[v].sr & 0xf0;
        osc[v].release = releases[sid.v[v].sr & 0xf];
        osc[v].wave = sid.v[v].wave;
        osc[v].freq = ((unsigned long) sid.v[v].freq) * freqmul;
    }

    filter.freq = (16 * sid.ffreqhi + (sid.ffreqlo & 0x7)) * filtmul;

    if (filter.freq > quickfloat_ConvertFromInt(1))
        filter.freq = quickfloat_ConvertFromInt(1);
    /* the above line isnt correct at all - the problem is that the filter
       works only up to rmxfreq/4 - this is sufficient for 44KHz but isnt
       for 32KHz and lower - well, but sound quality is bad enough then to
       neglect the fact that the filter doesnt come that high ;) */
    filter.l_ena = get_bit(sid.ftp_vol, 4);
    filter.b_ena = get_bit(sid.ftp_vol, 5);
    filter.h_ena = get_bit(sid.ftp_vol, 6);
    filter.v3ena = !get_bit(sid.ftp_vol, 7);
    filter.vol = (sid.ftp_vol & 0xf);
    filter.rez = quickfloat_ConvertFromFloat(1.2f) -
                 quickfloat_ConvertFromFloat(0.04f) * (sid.res_ftv >> 4);

    /* We precalculate part of the quick float operation, saves time in loop later */
    filter.rez >>= 8;


    /* now render the buffer */
    for (bp = 0; bp < len; bp++) {
        int32_t outo = 0;
        int32_t outf = 0;
        /* step 2 : generate the two output signals (for filtered and non-
                    filtered) from the osc/eg sections */
        for (v = 0; v < 3; v++) {
            if (!sid.v[v].muted) {
                /* update wave counter */
                osc[v].counter = (osc[v].counter + osc[v].freq) & 0xFFFFFFF;
                /* reset counter / noise generator if reset get_bit set */
                if (osc[v].wave & 0x08) {
                    osc[v].counter = 0;
                    osc[v].noisepos = 0;
                    osc[v].noiseval = 0xffffff;
                }
                unsigned char refosc = v ? v - 1 : 2;  /* reference oscillator for sync/ring */
                /* sync oscillator to refosc if sync bit set */
                if (osc[v].wave & 0x02)
                    if (osc[refosc].counter < osc[refosc].freq)
                        osc[v].counter = osc[refosc].counter * osc[v].freq / osc[refosc].freq;
                /* generate waveforms with really simple algorithms */
                unsigned char triout = (unsigned char) (osc[v].counter >> 19);
                if (osc[v].counter >> 27)
                    triout ^= 0xff;
                unsigned char sawout = (unsigned char) (osc[v].counter >> 20);
                unsigned char plsout = (unsigned char) ((osc[v].counter > osc[v].pulse) - 1);

                /* generate noise waveform exactly as the SID does. */
                if (osc[v].noisepos != (osc[v].counter >> 23)) {
                    osc[v].noisepos = osc[v].counter >> 23;
                    osc[v].noiseval = (osc[v].noiseval << 1) |
                                      (get_bit(osc[v].noiseval, 22) ^ get_bit(osc[v].noiseval, 17));
                    osc[v].noiseout = (get_bit(osc[v].noiseval, 22) << 7) |
                                      (get_bit(osc[v].noiseval, 20) << 6) |
                                      (get_bit(osc[v].noiseval, 16) << 5) |
                                      (get_bit(osc[v].noiseval, 13) << 4) |
                                      (get_bit(osc[v].noiseval, 11) << 3) |
                                      (get_bit(osc[v].noiseval, 7) << 2) |
                                      (get_bit(osc[v].noiseval, 4) << 1) |
                                      (get_bit(osc[v].noiseval, 2) << 0);
                }
                unsigned char nseout = osc[v].noiseout;

                /* modulate triangle wave if ringmod bit set */
                if (osc[v].wave & 0x04)
                    if (osc[refosc].counter < 0x8000000)
                        triout ^= 0xff;

                /* now mix the oscillators with an AND operation as stated in
                   the SID's reference manual - even if this is completely wrong.
                   well, at least, the $30 and $70 waveform sounds correct and there's
                   no real solution to do $50 and $60, so who cares. */

                unsigned char outv = 0xFF;
                if (osc[v].wave & 0x10) outv &= triout;
                if (osc[v].wave & 0x20) outv &= sawout;
                if (osc[v].wave & 0x40) outv &= plsout;
                if (osc[v].wave & 0x80) outv &= nseout;

                /* so now process the volume according to the phase and adsr values */
                switch (osc[v].envphase) {
                    case 0 : {                          /* Phase 0 : Attack */
                        osc[v].envval += osc[v].attack;
                        if (osc[v].envval >= 0xFFFFFF) {
                            osc[v].envval = 0xFFFFFF;
                            osc[v].envphase = 1;
                        }
                        break;
                    }
                    case 1 : {                          /* Phase 1 : Decay */
                        osc[v].envval -= osc[v].decay;
                        if ((signed int) osc[v].envval <= (signed int) (osc[v].sustain << 16)) {
                            osc[v].envval = osc[v].sustain << 16;
                            osc[v].envphase = 2;
                        }
                        break;
                    }
                    case 2 : {                          /* Phase 2 : Sustain */
                        if ((signed int) osc[v].envval != (signed int) (osc[v].sustain << 16)) {
                            osc[v].envphase = 1;
                        }
                        /* :) yes, thats exactly how the SID works. and maybe
                           a music routine out there supports this, so better
                           let it in, thanks :) */
                        break;
                    }
                    case 3 : {                          /* Phase 3 : Release */
                        osc[v].envval -= osc[v].release;
                        if (osc[v].envval < 0x40000) osc[v].envval = 0x40000;

                        /* the volume offset is because the SID does not
                           completely silence the voices when it should. most
                           emulators do so though and thats the main reason
                           why the sound of emulators is too, err... emulated :)  */
                        break;
                    }
                }


                /* now route the voice output to either the non-filtered or the
                   filtered channel and dont forget to blank out osc3 if desired */

                if (v < 2 || filter.v3ena) {
                    if (osc[v].filter)
                        outf += (((int) (outv - 0x80)) * osc[v].envval) >> 22;
                    else
                        outo += (((int) (outv - 0x80)) * osc[v].envval) >> 22;
                }
            }
        }

        int32_t digi_out = 0;
        route_digi_signal(pDigiDetector, &digi_out, &outf, &outo);

        /* step 3
         * so, now theres finally time to apply the multi-mode resonant filter
         * to the signal. The easiest thing ist just modelling a real electronic
         * filter circuit instead of fiddling around with complex IIRs or even
         * FIRs ...
         * it sounds as good as them or maybe better and needs only 3 MULs and
         * 4 ADDs for EVERYTHING. SIDPlay uses this kind of filter, too, but
         * Mage messed the whole thing completely up - as the rest of the
         * emulator.
         * This filter sounds a lot like the 8580, as the low-quality, dirty
         * sound of the 6581 is uuh too hard to achieve :) */

        filter.h = quickfloat_ConvertFromInt(outf) - (filter.b >> 8) * filter.rez - filter.l;
        filter.b += quickfloat_Multiply(filter.freq, filter.h);
        filter.l += quickfloat_Multiply(filter.freq, filter.b);

        outf = 0;

        if (filter.l_ena) outf += quickfloat_ConvertToInt(filter.l);
        if (filter.b_ena) outf += quickfloat_ConvertToInt(filter.b);
        if (filter.h_ena) outf += quickfloat_ConvertToInt(filter.h);

        int final_sample;
        if (mahoney_samples_detected(pDigiDetector)) {
            final_sample = digi_out;
        } else {
            final_sample = (filter.vol * (outo + outf));
        }

        *(buffer + bp) = map_sample(final_sample);
        cycles++;
    }
}

uint8_t routeSignal(int32_t voice_out, int32_t *outf, int32_t *outo, uint8_t voice_idx) {
    // regular routing
    if (osc[voice_idx].filter) {
        // route to filter
        (*outf) += voice_out;
        return 1;
    } else {
        // route directly to output
        (*outo) += voice_out;
        return 0;
    }
}

static inline void setmem(unsigned short addr, unsigned char value) {
    detect_sample(pDigiDetector, addr, value);
    const uint16_t reg = addr & 0x1f;

    sidPoke(reg, value);
    memWriteIO(addr, value);

    addr = 0xd400 | reg;
    memWriteIO(addr, value);
}

/*
* Poke a value into the sid register
*/
void sidPoke(int reg, unsigned char val) {
    int voice = 0;

    if ((reg >= 7) && (reg <= 13)) {
        voice = 1;
        reg -= 7;
    } else if ((reg >= 14) && (reg <= 20)) {
        voice = 2;
        reg -= 14;
    }

    switch (reg) {
        case 0: { /* Set frequency: Low byte */
            sid.v[voice].freq = (sid.v[voice].freq & 0xff00) + val;
            break;
        }
        case 1: { /* Set frequency: High byte */
            sid.v[voice].freq = (sid.v[voice].freq & 0xff) + (val << 8);
            break;
        }
        case 2: { /* Set pulse width: Low byte */
            sid.v[voice].pulse = (sid.v[voice].pulse & 0xff00) + val;
            break;
        }
        case 3: { /* Set pulse width: High byte */
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
            return memReadRAM(pc++);
        case _abs:
            ad = memReadRAM(pc++);
            ad |= 256 * memReadRAM(pc++);
            return memReadRAM(ad);
        case absx:
            ad = memReadRAM(pc++);
            ad |= 256 * memReadRAM(pc++);
            ad2 = ad + x;
            return memReadRAM(ad2);
        case absy:
            ad = memReadRAM(pc++);
            ad |= 256 * memReadRAM(pc++);
            ad2 = ad + y;
            return memReadRAM(ad2);
        case zp:
            ad = memReadRAM(pc++);
            return memReadRAM(ad);
        case zpx:
            ad = memReadRAM(pc++);
            ad += x;
            return memReadRAM(ad & 0xff);
        case zpy:
            ad = memReadRAM(pc++);
            ad += y;
            return memReadRAM(ad & 0xff);
        case indx:
            ad = memReadRAM(pc++);
            ad += x;
            ad2 = memReadRAM(ad & 0xff);
            ad++;
            ad2 |= memReadRAM(ad & 0xff) << 8;
            return memReadRAM(ad2);
        case indy:
            ad = memReadRAM(pc++);
            ad2 = memReadRAM(ad);
            ad2 |= memReadRAM((ad + 1) & 0xff) << 8;
            ad = ad2 + y;
            return memReadRAM(ad);
        case acc:
            return a;
    }
    return 0;
}

static inline void setaddr(int mode, unsigned char val) {
    unsigned short ad, ad2;
    switch (mode) {
        case _abs:
            ad = memReadRAM(pc - 2);
            ad |= 256 * memReadRAM(pc - 1);
            setmem(ad, val);
            return;
        case absx:
            ad = memReadRAM(pc - 2);
            ad |= 256 * memReadRAM(pc - 1);
            ad2 = ad + x;
            setmem(ad2, val);
            return;
        case zp:
            ad = memReadRAM(pc - 1);
            setmem(ad, val);
            return;
        case zpx:
            ad = memReadRAM(pc - 1);
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
            ad = memReadRAM(pc++);
            ad |= memReadRAM(pc++) << 8;
            setmem(ad, val);
            return;
        case absx:
            ad = memReadRAM(pc++);
            ad |= memReadRAM(pc++) << 8;
            ad2 = ad + x;
            setmem(ad2, val);
            return;
        case absy:
            ad = memReadRAM(pc++);
            ad |= memReadRAM(pc++) << 8;
            ad2 = ad + y;
            setmem(ad2, val);
            return;
        case zp:
            ad = memReadRAM(pc++);
            setmem(ad, val);
            return;
        case zpx:
            ad = memReadRAM(pc++);
            ad += x;
            setmem(ad & 0xff, val);
            return;
        case zpy:
            ad = memReadRAM(pc++);
            ad += y;
            setmem(ad & 0xff, val);
            return;
        case indx:
            ad = memReadRAM(pc++);
            ad += x;
            ad2 = memReadRAM(ad & 0xff);
            ad++;
            ad2 |= memReadRAM(ad & 0xff) << 8;
            setmem(ad2, val);
            return;
        case indy:
            ad = memReadRAM(pc++);
            ad2 = memReadRAM(ad);
            ad2 |= memReadRAM((ad + 1) & 0xff) << 8;
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
    return memReadRAM(0x100 + s);
}

static inline void branch(int flag) {
    signed char dist;
    dist = (signed char) getaddr(imm);
    wval = pc + dist;
    if (flag) pc = wval;
}

void cpuReset(void) {
    a = x = y = 0;
    p = 0;
    s = 255;
    pc = getaddr(0xfffc);
    cycles = 0;
}

static inline void cpuParse(void) {
    unsigned char opc = memReadRAM(pc++);
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
            pc = 0;           /* Just quit the emulation */
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
            wval = memReadRAM(pc++);
            wval |= 256 * memReadRAM(pc++);
            switch (addr) {
                case _abs:
                    pc = wval;
                    break;
                case ind:
                    pc = memReadRAM(wval);
                    pc |= 256 * memReadRAM(wval + 1);
                    break;
            }
            break;
        case jsr:
            push((pc + 1) >> 8);
            push((pc + 1));
            wval = memReadRAM(pc++);
            wval |= 256 * memReadRAM(pc++);
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

void cpuJSR(unsigned short npc, unsigned char na) {
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

bool cpuJSRWithWatchdog(unsigned short npc, unsigned char na) {
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

void c64Init(int nSampleRate) {
    synth_init(nSampleRate);
    cpuReset();
    memResetIO();
    pDigiDetector = get_digi_detector();
}

bool sid_load_from_file(TCHAR file_name[], struct sid_info *info) {
    FIL pFile;
    BYTE header[PSID_HEADER_SIZE];
    BYTE buffer[SID_LOAD_BUFFER_SIZE];
    UINT bytesRead;
    if (f_open(&pFile, file_name, FA_READ) != FR_OK) return false;
    if (f_read(&pFile, &header, PSID_HEADER_SIZE, &bytesRead) != FR_OK) return false;
    if (bytesRead < PSID_HEADER_SIZE) return false;
    unsigned char *pHeader = (unsigned char *) header;
    unsigned char data_file_offset = pHeader[7];

    bool is_rsid = (pHeader[0x00] != 0x50) ? true : false;
    uint8_t version = pHeader[0x05];

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

    uint16_t flags = (version > 1) ?
                     (((uint16_t) pHeader[0x77]) | (((uint16_t) pHeader[0x77]) << 8)) : 0x0;
    is_ntsc = (version >= 2) && (flags & 0x8); // NTSC bit
    bool is_compatible = ((version & 0x2) && ((flags & 0x2) == 0));
    memResetRAM(is_ntsc, !is_rsid);
    memResetKernelROM(0);
    reset_digi_detector(pDigiDetector, sysGetClockRate(), is_rsid, is_compatible);

    f_lseek(&pFile, data_file_offset + 2);
    uint16_t offset = info->load_addr;
    while (true) {
        f_read(&pFile, buffer, SID_LOAD_BUFFER_SIZE, &bytesRead);
        if (bytesRead == 0) break;
        memCopyToRAM(buffer, offset, bytesRead);
        offset += SID_LOAD_BUFFER_SIZE;
    }
    f_close(&pFile);

    if (info->play_addr == 0) {
        if (!cpuJSRWithWatchdog(info->init_addr, 0)) return false;
        info->play_addr = (memReadRAM(0xffff) << 8) | memReadRAM(0xfffe);
    }
    return true;
}

void setLineLevel(bool on) {
    if (on) {
        map_shift_bits = 4;
    } else {
        map_shift_bits = 2;
    }
}

bool getLineLevelOn() {
    return map_shift_bits == 4;
}