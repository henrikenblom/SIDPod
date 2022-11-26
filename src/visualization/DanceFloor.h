//
// Created by Henrik Enblom on 2022-11-21.
//

#ifndef SIDPOD_DANCEFLOOR_H
#define SIDPOD_DANCEFLOOR_H

#include "../display/include/ssd1306.h"
#include "../platform_config.h"
#include "kiss_fft.h"
#include "../PSIDCatalogEntry.h"

#define FIBONACCI_NUMBER_COUNT  20
#define SOUND_SPRITE_COUNT      (32 * 128)
#define FFT_SAMPLES             1024

class DanceFloor {
public:
    static void init(ssd1306_t *disp);

    static void start(PSIDCatalogEntry *entry);

    static void stop();

    struct StarSprite {
        int x;
        int y;
    };
    struct SoundSprite {
        int velocity;
        int distance;
        int frequency_bin;
    };
private:
    static void initFibonacci();

    static void drawHorizontalLine(uint8_t y);

    static void drawScroller();

    static void drawStarrySky();

    static void drawFibonacciLandscape();

    static void drawSoundSprite(SoundSprite sprite);

    static void updateSoundSprites();

    static void drawScene(kiss_fft_cpx *fft_out);

    static void visualize();
};

#endif //SIDPOD_DANCEFLOOR_H
