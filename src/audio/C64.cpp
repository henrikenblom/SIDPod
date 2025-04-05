/***************************************************************************
*
* CPU and memory routines are based on Rockbox (April 2006), which in turn
* is based on the TinySID engine written by Tammo Hinrichs (kb) and
* Rainer Sinsch 1998-1999.
*
***************************************************************************/

#include "C64.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include "../platform_config.h"
#include "reSID/sid.h"

#define ICODE_ATTR
#define IDATA_ATTR
#define ICONST_ATTR

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

/* ------------------------------------------------------ C64 Emu Stuff */
static long watchdog_counter = 0;

static unsigned char bval IDATA_ATTR;
static unsigned short wval IDATA_ATTR;
/* -------------------------------------------------- Register & memory */
static unsigned char a, x, y, s, p IDATA_ATTR;
static unsigned short pc IDATA_ATTR;

unsigned char memory[65536];

static constexpr int opcodes[256] ICONST_ATTR = {
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


static constexpr int modes[256] ICONST_ATTR = {
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

/* ------------------------------------------------------------- synthesis
   initialize SID and frequency dependant values */
void C64::synth_init() {
    reSID.reset();
    reSID.enable_filter(true);
    reSID.enable_external_filter(true);
    reSID.set_sampling_parameters(CLOCKFREQ, SAMPLE_FAST, SAMPLE_RATE);
}

void C64::sid_synth_render(short *buffer, size_t len) {
    cycle_count delta_t = static_cast<cycle_count>(static_cast<float>(CLOCKFREQ)) / (
                              static_cast<float>(SAMPLE_RATE) / static_cast<float>(len));
    reSID.clock(delta_t, buffer, static_cast<int>(len));
}

/*
* C64 Mem Routines
*/
static unsigned char getmem(unsigned short addr) {
    return memory[addr];
}

static void setmem(unsigned short addr, unsigned char value) {
    if ((addr & 0xfc00) == 0xd400) {
        C64::sidPoke(addr & 0x1f, value);
    } else {
        memory[addr] = value;
    }
}

/*
* Poke a value into the sid register
*/
void C64::sidPoke(int reg, unsigned char val) {
    reSID.write(reg, val);
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
        default:
            return 0;
    }
}

static void setaddr(int mode, unsigned char val) {
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
        default: ;
    }
}


static void putaddr(int mode, unsigned char val) {
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
        default: ;
    }
}


static void setflags(int flag, int cond) {
    if (cond) p |= flag;
    else p &= ~flag;
}


static void push(unsigned char val) {
    setmem(0x100 + s, val);
    if (s) s--;
}

static unsigned char pop() {
    if (s < 0xff) s++;
    return getmem(0x100 + s);
}

static void branch(int flag) {
    signed char dist;
    dist = (signed char) getaddr(imm);
    wval = pc + dist;
    if (flag) pc = wval;
}

void C64::cpuReset() {
    a = x = y = 0;
    p = 0;
    s = 255;
    pc = getaddr(0xfffc);
}

static void cpuParse() {
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
            wval = static_cast<unsigned short>(a) - bval;
            setflags(FLAG_Z, !wval);
            setflags(FLAG_N, wval & 0x80);
            setflags(FLAG_C, a >= bval);
            break;
        case cpx:
            bval = getaddr(addr);
            wval = static_cast<unsigned short>(x) - bval;
            setflags(FLAG_Z, !wval);
            setflags(FLAG_N, wval & 0x80);
            setflags(FLAG_C, x >= bval);
            break;
        case cpy:
            bval = getaddr(addr);
            wval = static_cast<unsigned short>(y) - bval;
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
                default: ;
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
        default: ;
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

void C64::c64Init() {
    synth_init();
    memset(memory, 0, sizeof(memory));
    cpuReset();
}

bool C64::sid_load_from_file(TCHAR file_name[], struct sid_info *info) {
    FIL pFile;
    BYTE header[SID_HEADER_SIZE];
    BYTE buffer[SID_LOAD_BUFFER_SIZE];
    UINT bytesRead;
    if (f_open(&pFile, file_name, FA_READ) != FR_OK) return false;
    if (f_read(&pFile, &header, SID_HEADER_SIZE, &bytesRead) != FR_OK) return false;
    if (bytesRead < SID_HEADER_SIZE) return false;
    auto *pHeader = static_cast<unsigned char *>(header);
    // TODO: Rewrite, similar to the PSID class in libsidplayfp, so that speed, chip model/count etc are properly set.
    unsigned char data_file_offset = pHeader[7];

    info->rsid = (pHeader[3] | pHeader[2] << 0x08 | pHeader[1] << 0x10 | pHeader[0] << 0x18) == RSID_ID;

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
    //reSID.enable_filter(on);
}

bool C64::getLineLevelOn() {
    return false;
}
