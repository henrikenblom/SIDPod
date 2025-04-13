#include "DanceFloor.h"
#include "../platform_config.h"
#include "kiss_fftr.h"
#include "../System.h"
#include "../audio/SIDPlayer.h"
#include "../audio/C64.h"
#include "../UI.h"

char experience[20];

namespace Visualization {
    DanceFloor::DanceFloor(ssd1306_t *_pDisp) {
        pDisp = _pDisp;
        fibonacci[0] = 0;
        fibonacci[1] = 1;
        for (int i = 2; i < HORIZONTAL_LANDSCAPE_LINES; i++) {
            fibonacci[i] = fibonacci[i - 1] + fibonacci[i - 2];
        }
        fft_cfg = kiss_fftr_alloc(FFT_SAMPLES, false, nullptr, nullptr);
    }

    void DanceFloor::drawDottedHorizontalLine(uint8_t y) {
        if (y <= DISPLAY_HEIGHT) {
            for (int x = 0; x < DISPLAY_WIDTH - 1; x += 2) {
                ssd1306_draw_pixel(pDisp, x + horizontalLineDitherOffset, y);
            }
        }
        horizontalLineDitherOffset ^= 1;
    }

    void DanceFloor::drawHorizontalLine(uint8_t y) const {
        if (y <= DISPLAY_HEIGHT) {
            ssd1306_draw_line(pDisp, 0, y, DISPLAY_WIDTH - 1, y);
        }
    }

    void DanceFloor::drawScroller() {
        if (showScroller) {
            ssd1306_draw_string(pDisp, rsOffset--, 1, 1, scrollText);
            if (rsOffset < SCROLL_LIMIT) {
                rsOffset = DISPLAY_WIDTH + 1;
                transition = FROM_SPECTRUM;
                millisSinceLastSceneChange = millis_now();
            }
        }
    }

    void DanceFloor::drawStarrySky(bool ignoreHorizon) {
        for (auto &sprite: starSprites) {
            if (random() % 100 != 1) {
                if (ignoreHorizon || sprite.y < horizon) {
                    ssd1306_draw_pixel(pDisp, sprite.x, sprite.y);
                } else {
                    drawHorizonPixel(sprite.x, sprite.y);
                }
            }
        }
    }

    void DanceFloor::drawHorizonPixel(int32_t x, int32_t y) const {
        if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT && y < horizon) {
            ssd1306_draw_pixel(pDisp, x, y);
        }
    }

    void DanceFloor::drawCircle(const int32_t x, const int32_t y, const int32_t radius) const {
        int32_t f = 1 - radius;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * radius;
        int32_t xi = 0;
        int32_t yi = radius;

        drawHorizonPixel(x, y + radius);
        drawHorizonPixel(x, y - radius);
        drawHorizonPixel(x + radius, y);
        drawHorizonPixel(x - radius, y);

        while (xi < yi) {
            if (f >= 0) {
                yi--;
                ddF_y += 2;
                f += ddF_y;
            }
            xi++;
            ddF_x += 2;
            f += ddF_x;

            drawHorizonPixel(x + xi, y + yi);
            drawHorizonPixel(x - xi, y + yi);
            drawHorizonPixel(x + xi, y - yi);
            drawHorizonPixel(x - xi, y - yi);
            drawHorizonPixel(x + yi, y + xi);
            drawHorizonPixel(x - yi, y + xi);
            drawHorizonPixel(x + yi, y - xi);
            drawHorizonPixel(x - yi, y - xi);
        }
    }

    void DanceFloor::drawFilledCircle(int32_t x, int32_t y, int32_t radius) const {
        for (int32_t i = -radius; i <= radius; i++) {
            for (int32_t j = -radius; j <= radius; j++) {
                if (i * i + j * j <= radius * radius) {
                    ssd1306_draw_pixel(pDisp, x + i, y + j);
                }
            }
        }
    }

    void DanceFloor::drawFibonacciLandscape() {
        drawDottedHorizontalLine(horizon);
        for (const unsigned short i: fibonacci) {
            drawDottedHorizontalLine(horizon + i / (20 - rvOffset));
        }
        if (horizon < 20) {
            ssd1306_draw_line(pDisp, 0, 25, 23, horizon);
            ssd1306_draw_line(pDisp, 42, 31, 48, horizon);
            ssd1306_draw_line(pDisp, 79, horizon, 86, 31);
            ssd1306_draw_line(pDisp, 104, horizon, 127, 25);
        }
        if (rvOffset++ > 4) rvOffset = 0;
    }

    void DanceFloor::drawSoundSprite(SoundSprite sprite) const {
        int x;
        double f = 0.012;
        int y1 = 39 - sprite.distance - sprite.velocity - (20 - horizon);
        int y2 = sprite.distance < 7
                     ? (int) (y1 + sprite.velocity * 0.4)
                     : (int) (y1 + sprite.velocity + (sprite.distance * 0.29));
        int perspectiveComp;
        if (sprite.frequency_bin <= displayCenter) {
            perspectiveComp = static_cast<int>((displayCenter - sprite.frequency_bin) * (sprite.distance * f));
            x = sprite.frequency_bin + perspectiveComp;
        } else {
            perspectiveComp = static_cast<int>((sprite.frequency_bin - displayCenter) * (sprite.distance * f));
            x = sprite.frequency_bin - perspectiveComp;
        }
        ssd1306_draw_line(pDisp, x, std::max(0, y1), x, std::min(DISPLAY_HEIGHT - 1, y2));
    }

    void DanceFloor::updateSoundSprites() {
        for (auto &soundSprite: soundSprites) {
            auto sprite = soundSprite;
            if (sprite.distance > -16) {
                drawSoundSprite(sprite);
                sprite.distance -= 4;
            }
            soundSprite = sprite;
        }
    }

    void DanceFloor::updateRoundSprites() {
        for (auto &roundSprite: roundSprites) {
            auto sprite = roundSprite;
            if (sprite.distance > -16 && sprite.y < horizon) {
                drawCircle(sprite.x, sprite.y, sprite.distance / 2.5 - (40 - horizon));
                sprite.distance -= 4;
                sprite.x -= starFieldVisible ? 2 : 0;
            }
            roundSprite = sprite;
        }
    }

    void DanceFloor::updateStarFieldSprites() {
        for (auto &starFieldSprite: starFieldSprites) {
            auto sprite = starFieldSprite;
            if (sprite.x > 0) {
                drawHorizonPixel(sprite.x, sprite.y);
                sprite.x -= sprite.velocity;
            }
            starFieldSprite = sprite;
        }
    }

    // TODO: Refactor this to use the same logic as the other sprites and make sure it's reset after each run
    void DanceFloor::drawStarShip() {
        if (starShipX > -20) {
            ssd1306_draw_line(pDisp, starShipX, starShipY, starShipX + 7, starShipY); // Left gondola
            ssd1306_draw_pixel(pDisp, starShipX + 5, starShipY + 1);
            ssd1306_draw_pixel(pDisp, starShipX + 6, starShipY + 2);
            ssd1306_draw_line(pDisp, starShipX + 5, starShipY + 3, starShipX + 10, starShipY + 3); // Body
            ssd1306_draw_pixel(pDisp, starShipX + 6, starShipY + 4);
            ssd1306_draw_pixel(pDisp, starShipX + 5, starShipY + 5);
            ssd1306_draw_pixel(pDisp, starShipX + 5, starShipY + 5);
            ssd1306_draw_line(pDisp, starShipX, starShipY + 6, starShipX + 7, starShipY + 6); // Right gondola
            drawFilledCircle(starShipX + 15, starShipY + 3, 4);
        }
        starShipX += starShipVelocity;
        starShipY += starShipVelocity * 0.05;
        if (!letStarShipRoam && starShipX > DISPLAY_WIDTH / 2 + 10) {
            starShipVelocity = -0.2;
        } else if (starShipX < -65) {
            starShipVelocity = 3.0;
            letStarShipRoam = true;
        }
        if (letStarShipRoam) {
            starShipVelocity += 0.2;
        }
    }

    bool DanceFloor::shouldUpdateRoundSprites() const {
        return alternativeScene || transition == FROM_SPECTRUM || transition == FROM_ALTERNATIVE;
    }

    bool DanceFloor::shouldUpdateSoundSprites() const {
        return !alternativeScene || transition != NO_TRANSITION;
    }

    bool DanceFloor::shouldUpdateStarFieldSprites() const {
        return starFieldVisible;
    }

    bool DanceFloor::isWithinStarFieldTimeWindow() const {
        return millis_now() >= millisSinceLastSceneChange + STARFIELD_ACTIVE_AFTER &&
               millis_now() < millisSinceLastSceneChange + ALTERNATIVE_SCENE_DURATION + 9000;
    }

    bool DanceFloor::isOutsideOfRoundSpriteTimeWindow() const {
        return millis_now() >= millisSinceLastSceneChange + STARFIELD_ACTIVE_AFTER + 6000 &&
               millis_now() < millisSinceLastSceneChange + ALTERNATIVE_SCENE_DURATION - 6000;
    }

    void DanceFloor::drawScene(const kiss_fft_cpx *fft_out) {
        for (uint8_t x = LOW_FREQ_DOMINANCE_COMP_OFFSET; x < 127 + LOW_FREQ_DOMINANCE_COMP_OFFSET; x++) {
            const int i = static_cast<int>(1.8) * x;
            int y = static_cast<int>((fft_out[i].r + fft_out[i].i +
                                      fft_out[i + 1].r + fft_out[i + 1].i + static_cast<float>(i)) *
                                     compFactor);
            if (y > 0) {
                if (y > 28) y /= 8;

                if (!isOutsideOfRoundSpriteTimeWindow() && shouldUpdateRoundSprites()) {
                    if (x % 16 == 0 || x == 0 || x == 127) {
                        RoundSprite roundSprite = {
                            .distance = 20, .x = random() % DISPLAY_WIDTH,
                            .y = random() % DISPLAY_HEIGHT
                        };
                        roundSprites[roundSpriteIndex++] = roundSprite;
                        if (roundSpriteIndex > ROUND_SPRITE_COUNT) roundSpriteIndex = 0;
                    }
                }
                if (shouldUpdateSoundSprites()) {
                    SoundSprite sprite = {
                        .velocity = static_cast<int8_t>(std::min(16,
                                                                 y)),
                        .distance = 20, .frequency_bin = static_cast<uint8_t>(x - LOW_FREQ_DOMINANCE_COMP_OFFSET)
                    };
                    soundSprites[sprite_index++] = sprite;
                    if (sprite_index > SOUND_SPRITE_COUNT) sprite_index = 0;
                }
                if (shouldUpdateStarFieldSprites()) {
                    if ((x % 64 == 0 || x == 0 || x == 127) && y > 0) {
                        StarFieldSprite starFieldSprite = {
                            .x = DISPLAY_WIDTH + 10, .y = (uint8_t) (random() % DISPLAY_HEIGHT),
                            .velocity = static_cast<int8_t>(std::min(8,
                                                                     y / 2))
                        };
                        starFieldSprites[starFieldSpriteIndex++] = starFieldSprite;
                        if (starFieldSpriteIndex > DISPLAY_WIDTH) starFieldSpriteIndex = 0;
                    }
                }
            }
        }

        ssd1306_clear(pDisp);
        if (shouldUpdateRoundSprites()) {
            updateRoundSprites();
        }
        if (shouldUpdateSoundSprites()) {
            drawFibonacciLandscape();
            drawStarrySky(false);
            if (transition == NO_TRANSITION) {
                drawScroller();
            }
            updateSoundSprites();
        }
        if (shouldUpdateStarFieldSprites()) {
            updateStarFieldSprites();
            drawStarShip();
        }
        ssd1306_show(pDisp);

        if (alternativeScene && millis_now() >= millisSinceLastSceneChange + ALTERNATIVE_SCENE_DURATION) {
            millisSinceLastSceneChange = millis_now();
            transition = FROM_ALTERNATIVE;
        }
        if (alternativeScene && !starFieldVisible && isWithinStarFieldTimeWindow()) {
            starFieldVisible = true;
        }
        if (transition == FROM_SPECTRUM) {
            horizon += 0.2;
            if (horizon >= 40) {
                transition = NO_TRANSITION;
                alternativeScene = true;
            }
        } else if (transition == FROM_ALTERNATIVE || transition == FROM_BEGIN) {
            horizon -= 0.2;
            if (horizon <= 10) {
                transition = NO_TRANSITION;
                alternativeScene = false;
                starFieldVisible = false;
                starShipX = -64;
                starShipY = DISPLAY_HEIGHT / 2 - 8;
            }
        }
    }


    void DanceFloor::randomizeExperience(char *experience) {
        const char *experiences[] = {"listening to", "enjoying", "experiencing", "feeling"};
        int randomIndex = rand() % 4; // NOLINT(*-msc50-cpp)
        strcpy(experience, experiences[randomIndex]);
    }

    void DanceFloor::visualize() {
        while (running) {
            if (!showScroller && strcmp(selectedEntry->fileName, SIDPlayer::getCurrentlyLoaded()->fileName) == 0) {
                SidInfo *entry = SIDPlayer::getSidInfo();
                randomizeExperience(experience);
                char extraText[50];
                if (entry->sidChipBase3) {
                    sprintf(extraText, "Did you know that this song uses three SID chips?");
                } else if (entry->sidChipBase2) {
                    sprintf(extraText, "Fun fact: This song uses two SID chips!");
                }
                snprintf(scrollText, sizeof(scrollText),
                         "This is %s by %s (%s) and you are %s %s on a SIDPod. %s %s",
                         entry->name, entry->author, entry->released, experience,
                         entry->isPSID ? "it" : "this RSID",
                         extraText,
                         random() % 4 == 1 ? "Enjoy!" : "");
                showScroller = true;
            }
            compFactor = SIDPlayer::lineLevelOn() ? LINE_LEVEL_SPECTRUM_COMPENSATION : DEFAULT_SPECTRUM_COMPENSATION;
            if (SIDPlayer::isPlaying()) {
                freeze = false;
                int j = 0;
                for (int i = 0; i < FFT_SAMPLES; i += 1) {
                    fftIn[j++] = visualizationBuffer[i];
                }
                kiss_fftr(fft_cfg, fftIn, fftOut);

                drawScene(fftOut);
            } else if (!SIDPlayer::loadingWasSuccessful()) {
                stop();
            } else if (!freeze) {
                ssd1306_clear(pDisp);
                drawStarrySky(true);
                ssd1306_show(pDisp);
                busy_wait_ms(500);
                if (!SIDPlayer::isPlaying()) {
                    if (!SIDPlayer::loadingWasSuccessful()) {
                        UI::drawDialog(failedLabel);
                    } else {
                        UI::drawDialog(pausedLabel);
                    }
                    ssd1306_show(pDisp);
                    freeze = true;
                }
            }
            System::virtualVBLSync();
        }
    }

    void DanceFloor::start(PlaylistEntry *_selectedEntry) {
        running = true;
        freeze = false;
        for (int i = 0; i < SIDPLAYER_STARTUP_GRACE_TIME; i++) {
            if (SIDPlayer::isPlaying()) break;
            busy_wait_ms(1);
        }
        if (selectedEntry != _selectedEntry) {
            selectedEntry = _selectedEntry;
            rsOffset = DISPLAY_WIDTH + 32;
            rvOffset = 0;
            showScroller = false;
            for (auto &soundSprite: soundSprites) {
                soundSprite = {0};
            }
            for (auto &roundSprite: roundSprites) {
                roundSprite = {0};
            }
            for (auto &startFieldSprite: starFieldSprites) {
                startFieldSprite = {0};
            }

            alternativeScene = false;
            starFieldVisible = false;
            starShipX = -64;
            starShipY = DISPLAY_HEIGHT / 2 - 8;
            letStarShipRoam = false;
            transition = FROM_BEGIN;
            horizon = 32;
        }
        visualize();
    }

    void DanceFloor::stop() {
        running = false;
    }
}
