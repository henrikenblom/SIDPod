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

        struct RoundSprite {
            int8_t distance;
            int32_t x;
            int32_t y;
        };

        enum Transition {
            NO_TRANSITION,
            FROM_ALTERNATIVE,
            FROM_SPECTRUM,
        };

    private:
        void drawDottedHorizontalLine(uint8_t y);

        void drawHorizontalLine(uint8_t y) const;

        void drawScroller();

        void drawStarrySky();

        void drawHorizonPixel(int32_t x, int32_t y) const;

        void drawCircle(int32_t x, int32_t y, int32_t radius) const;

        void drawArc(int32_t x, int32_t y, int32_t radius, int32_t startAngle, int32_t endAngle) const;

        void drawFibonacciLandscape();

        void drawSoundSprite(SoundSprite sprite) const;

        void drawPausedLabel();

        void updateSoundSprites();

        void updateRoundSprites();

        void drawScene(kiss_fft_cpx *fft_out);

        static void randomizeExperience(char *experience);

        void visualize();

        uint32_t millis_now() {
            return to_ms_since_boot(get_absolute_time());
        }

        int sprite_index = 0;
        int roundSpriteIndex = 0;
        char scrollText[160]{};
        char pausedLabel[7] = "PAUSED";
        uint8_t horizontalLineDitherOffset = 0;
        int rvOffset = 0;
        int rsOffset = DISPLAY_WIDTH + 32;
        int displayCenter = DISPLAY_WIDTH / 2;
        volatile bool running = false;
        volatile bool freeze = false;
        volatile bool showScroller = false;
        kiss_fftr_cfg fft_cfg{};
        double compFactor = DEFAULT_SPECTRUM_COMPENSATION;
        CatalogEntry *selectedEntry{};
        bool alternativeScene = false;
        Transition transition = NO_TRANSITION;
        uint32_t millisSinceLastSceneChange = millis_now();
        float horizon = 10;

        void (*stopCallback)() = nullptr;

        SoundSprite soundSprites[SOUND_SPRITE_COUNT]{};
        RoundSprite roundSprites[DISPLAY_WIDTH / 2]{};
        StarSprite starSprites[12] = {
            {6, 4},
            {14, 0},
            {28, 8},
            {34, 5},
            {42, 9},
            {60, 4},
            {72, 3},
            {80, 6},
            {96, 1},
            {108, 5},
            {116, 8},
            {124, 0}
        };
        uint16_t fibonacci[HORIZONTAL_LANDSCAPE_LINES]{};
        kiss_fft_scalar fftIn[FFT_SAMPLES]{};
        kiss_fft_cpx fftOut[FFT_SAMPLES]{};
        ssd1306_t *pDisp;
    };
}
#endif //SIDPOD_DANCEFLOOR_H
