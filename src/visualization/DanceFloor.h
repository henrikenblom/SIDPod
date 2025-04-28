#ifndef SIDPOD_DANCEFLOOR_H
#define SIDPOD_DANCEFLOOR_H

#include "../display/include/ssd1306.h"
#include "../platform_config.h"
#include "kiss_fft.h"
#include "../Playlist.h"
#include "kiss_fftr.h"

namespace Visualization {
    class DanceFloor {
    public:
        explicit DanceFloor(ssd1306_t *disp);

        void start();

        void stop();

        void init();

        struct StarSprite {
            uint8_t x;
            uint8_t y;
        };

        struct StarFieldSprite {
            uint8_t x;
            uint8_t y;
            int8_t velocity;
        };

        struct SoundSprite {
            int8_t velocity;
            int8_t distance;
            uint8_t frequency_bin;
        };

        struct RoundSprite {
            int8_t distance;
            float x;
            int32_t y;
        };

        enum Transition {
            NO_TRANSITION,
            FROM_ALTERNATIVE,
            FROM_SPECTRUM,
            FROM_BEGIN,
            FROM_SPHERE
        };

    private:
        void drawDottedHorizontalLine(uint8_t y);

        void drawHorizontalLine(uint8_t y) const;

        void drawScroller();

        void drawStarrySky(bool ignoreHorizon);

        void drawHorizonPixel(int32_t x, int32_t y) const;

        void drawCircle(int32_t x, int32_t y, int32_t radius) const;

        void drawFilledCircle(int32_t x, int32_t y, int32_t radius) const;

        void drawFibonacciLandscape();

        void drawSoundSprite(SoundSprite sprite) const;

        bool shouldUpdateRoundSprites() const;

        bool shouldUpdateSoundSprites() const;

        bool shouldUpdateStarFieldSprites() const;

        bool shouldUpdateSphere() const;

        bool isWithinStarFieldTimeWindow() const;

        bool isOutsideOfRoundSpriteTimeWindow() const;

        void showCurrentSongNumber(bool show, bool hide);

        void draw3DPixelSphere(int32_t x, int32_t y, float size, int16_t xAxisRotation, int16_t yAxisRotation) const;

        void drawSphereScene(int totalY);

        void draw3DPixelBall(int32_t x, int32_t y, int32_t size, int16_t xAxisRotation, int16_t yAxisRotation) const;

        void updateSoundSprites();

        void updateRoundSprites(bool instantImplosion);

        void updateStarFieldSprites();

        void drawStarShip();

        void drawScene(const kiss_fft_cpx *fft_out);

        static void randomizeExperience(char *experience);

        void visualize();

        uint32_t millis_now() const {
            return to_ms_since_boot(get_absolute_time());
        }

        float roundSpriteXVelocity = 0;
        float roundSpriteTargetXVelocity = 0;
        int sprite_index = 0;
        int roundSpriteIndex = 0;
        int starFieldSpriteIndex = 0;
        float starShipX = -64;
        float starShipY = DISPLAY_HEIGHT / 2 - 8;
        float starShipVelocity = 0.3;
        bool letStarShipRoam = false;
        char scrollText[200]{};
        char pausedLabel[7] = "PAUSED";
        char failedLabel[7] = "FAILED";
        uint8_t horizontalLineDitherOffset = 0;
        int rvOffset = 0;
        int rsOffset = DISPLAY_WIDTH + 32;
        int displayCenter = DISPLAY_WIDTH / 2;
        volatile bool running = false;
        volatile bool freeze = false;
        volatile bool showScroller = false;
        kiss_fftr_cfg fft_cfg{};
        double compFactor = DEFAULT_SPECTRUM_COMPENSATION;
        bool alternativeScene = false;
        bool starFieldVisible = false;
        bool sphereScene = false;
        Transition transition = NO_TRANSITION;
        uint32_t millisSinceLastSceneChange = millis_now();
        float horizon = 10;
        float songNumberOffset = 0;
        uint16_t sphereRotationX = 0;
        uint16_t sphereRotationY = 0;
        uint16_t sphereSize = 100;
        uint16_t sphereSizeInc = 1;
        uint16_t sphereX = DISPLAY_X_CENTER;
        uint16_t sphereXInc = 1;
        uint16_t sphereZComp = 0;
        uint8_t minSphereSize = 6;
        uint8_t maxSphereSize = 64;

        void (*stopCallback)() = nullptr;

        SoundSprite soundSprites[SOUND_SPRITE_COUNT]{};
        RoundSprite roundSprites[DISPLAY_WIDTH / 2]{};
        StarFieldSprite starFieldSprites[STARFIELD_SPRITE_COUNT]{};
        StarSprite starSprites[24] = {
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
            {124, 0},
            {2, 14},
            {18, 10},
            {20, 18},
            {30, 25},
            {52, 19},
            {62, 24},
            {70, 30},
            {86, 16},
            {100, 11},
            {109, 15},
            {114, 18},
            {127, 10}
        };
        uint16_t fibonacci[HORIZONTAL_LANDSCAPE_LINES]{};
        kiss_fft_scalar fftIn[FFT_SAMPLES]{};
        kiss_fft_cpx fftOut[FFT_SAMPLES]{};
        ssd1306_t *pDisp;
    };
}
#endif //SIDPOD_DANCEFLOOR_H
