#ifndef SIDPOD_DANCEFLOOR_H
#define SIDPOD_DANCEFLOOR_H

#include "../display/include/ssd1306.h"
#include "../platform_config.h"
#include "kiss_fft.h"
#include "../PSIDCatalog.h"
#include "kiss_fftr.h"

namespace Visualization {
    class DanceFloor {
    public:
        explicit DanceFloor(ssd1306_t *disp);

        void start(CatalogEntry *selectedEntry);

        void stop();

        struct StarSprite {
            uint8_t x;
            uint8_t y;
        };
        struct SoundSprite {
            int8_t velocity;
            int8_t distance;
            uint8_t frequency_bin;
        };
    private:
        void drawHorizontalLine(uint8_t y);

        void drawScroller();

        void drawStarrySky();

        void drawFibonacciLandscape();

        void drawSoundSprite(SoundSprite sprite) const;

        void drawPausedLabel();

        void updateSoundSprites();

        void drawScene(kiss_fft_cpx *fft_out);

        void visualize();

        int sprite_index = 0;
        char scrollText[160]{};
        char pausedLabel[7] = "PAUSED";
        int16_t scrollLimit = -1600;
        uint8_t horizontalLineDitherOffset = 0;
        int rvOffset = 0;
        int rsOffset = DISPLAY_WIDTH + 32;
        int displayCenter = DISPLAY_WIDTH / 2;
        bool running = false;
        bool freeze = false;
        bool showScroller = false;
        kiss_fftr_cfg fft_cfg{};
        double compFactor = DEFAULT_SPECTRUM_COMPENSATION;
        CatalogEntry *selectedEntry{};

        void (*stopCallback)() = nullptr;

        DanceFloor::SoundSprite soundSprites[SOUND_SPRITE_COUNT]{};
        DanceFloor::StarSprite starSprites[12] = {{6,   4},
                                                  {14,  0},
                                                  {28,  8},
                                                  {34,  5},
                                                  {42,  9},
                                                  {60,  4},
                                                  {72,  3},
                                                  {80,  6},
                                                  {96,  1},
                                                  {108, 5},
                                                  {116, 8},
                                                  {124, 0}};
        uint16_t fibonacci[HORIZONTAL_LANDSCAPE_LINES]{};
        kiss_fft_scalar fftIn[FFT_SAMPLES]{};
        kiss_fft_cpx fftOut[FFT_SAMPLES]{};
        ssd1306_t *pDisp;
    };
}
#endif //SIDPOD_DANCEFLOOR_H
