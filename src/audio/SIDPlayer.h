//
// Created by Henrik Enblom on 2022-11-14.
//

#ifndef SIDPOD_SIDPLAYER_H
#define SIDPOD_SIDPLAYER_H

#include "../PSIDCatalogEntry.h"
#include "pico/audio_i2s.h"

#define NEW_AUDIO_DATA_AVAILABLE    123
#define AUDIO_RENDERING_STARTED     124

#define SAMPLE_RATE             ((uint32_t)48000)
#define SAMPLES_PER_BUFFER      (12 * 1024)

extern "C" bool sid_load_from_memory(void *data, size_t size, struct sid_info *info);
extern "C" void sid_synth_render(uint16_t *buffer, size_t len);
extern "C" void cpuJSR(unsigned short, unsigned char);
extern "C" void sidPoke(int reg, unsigned char val);
extern "C" void c64Init(int nSampleRate);
extern "C" unsigned char memory[];
extern "C" struct sid_info {
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
extern uint16_t intermediateBuffer[];

class SIDPlayer {
public:
    static void initAudio();

    static bool loadPSID(PSIDCatalogEntry psidFile);

    static bool play();

    static void stop();

private:

    static void generateSamples();

    [[noreturn]] static void sampleRenderingLoop();
};


#endif //SIDPOD_SIDPLAYER_H
