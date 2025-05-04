#include "DanceFloor.h"

#include "Catalog.h"
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
                showScroller = false;
            }
        }
    }

    void DanceFloor::drawStarrySky(bool ignoreHorizon) {
        for (auto &sprite: starSprites) {
            if (random() % 100 != 1) {
                if (ignoreHorizon || sprite.y < horizon) {
                    ssd1306_draw_pixel(pDisp, sprite.x, sprite.y);
                } else {
                    drawHorizonAwarePixel(sprite.x, sprite.y);
                }
            }
        }
    }

    void DanceFloor::drawHorizonAwarePixel(int32_t x, int32_t y) const {
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

        drawHorizonAwarePixel(x, y + radius);
        drawHorizonAwarePixel(x, y - radius);
        drawHorizonAwarePixel(x + radius, y);
        drawHorizonAwarePixel(x - radius, y);

        while (xi < yi) {
            if (f >= 0) {
                yi--;
                ddF_y += 2;
                f += ddF_y;
            }
            xi++;
            ddF_x += 2;
            f += ddF_x;

            drawHorizonAwarePixel(x + xi, y + yi);
            drawHorizonAwarePixel(x - xi, y + yi);
            drawHorizonAwarePixel(x + xi, y - yi);
            drawHorizonAwarePixel(x - xi, y - yi);
            drawHorizonAwarePixel(x + yi, y + xi);
            drawHorizonAwarePixel(x - yi, y + xi);
            drawHorizonAwarePixel(x + yi, y - xi);
            drawHorizonAwarePixel(x - yi, y - xi);
        }
    }

    void DanceFloor::drawFilledCircle(int32_t x, int32_t y, int32_t radius) const {
        for (int32_t i = -radius; i <= radius; i++) {
            for (int32_t j = -radius; j <= radius; j++) {
                if (i * i + j * j <= radius * radius) {
                    drawHorizonAwarePixel(x + i, y + j);
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

    void DanceFloor::updateRoundSprites(bool instantImplosion) {
        for (auto &roundSprite: roundSprites) {
            auto sprite = roundSprite;
            int32_t radius = sprite.distance / 2.5 - (40 - (instantImplosion ? 10 : horizon));
            if (sprite.distance > -16 && sprite.y < horizon) {
                drawCircle(sprite.x, sprite.y, radius);
                sprite.distance -= 4;
                sprite.x += roundSpriteXVelocity;
            }
            roundSprite = sprite;
        }
        if (roundSpriteTargetXVelocity < 0 && roundSpriteXVelocity > roundSpriteTargetXVelocity) {
            roundSpriteXVelocity -= 0.2;
        } else if (roundSpriteXVelocity < roundSpriteTargetXVelocity) {
            roundSpriteXVelocity += 0.2;
        }
    }

    void DanceFloor::updateStarFieldSprites() {
        for (auto &starFieldSprite: starFieldSprites) {
            auto sprite = starFieldSprite;
            if (sprite.x > 0) {
                drawHorizonAwarePixel(sprite.x, sprite.y);
                sprite.x -= sprite.velocity;
            }
            starFieldSprite = sprite;
        }
    }

    // TODO: Refactor this to use the same logic as the other sprites
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
        return sphereSizeInc != 0 && (alternativeScene || transition == FROM_SPECTRUM || transition ==
                                      FROM_ALTERNATIVE);
    }

    bool DanceFloor::shouldUpdateSoundSprites() const {
        return (!alternativeScene && !sphereScene) || transition == FROM_SPHERE;;
    }

    bool DanceFloor::shouldUpdateStarFieldSprites() const {
        return starFieldVisible;
    }

    bool DanceFloor::shouldUpdateSphere() const {
        return sphereScene || transition == FROM_SPHERE || transition == FROM_ALTERNATIVE;
    }

    bool DanceFloor::isWithinStarFieldTimeWindow() const {
        return System::System::millis_now() >= lastSceneChangeMS + STARFIELD_ACTIVE_AFTER &&
               System::System::millis_now() < lastSceneChangeMS + ALTERNATIVE_SCENE_DURATION + 9000;
    }

    bool DanceFloor::isOutsideOfRoundSpriteTimeWindow() const {
        return System::System::millis_now() >= lastSceneChangeMS + STARFIELD_ACTIVE_AFTER + 6000;
    }

    void DanceFloor::showCurrentSongNumber(bool show, bool hide) {
        const int currentSong = SIDPlayer::getCurrentSong();
        const int songCount = SIDPlayer::getSongCount();
        char songNumber[14];
        snprintf(songNumber, sizeof(songNumber), "Song %d/%d", currentSong + 1, songCount);
        const int width = static_cast<int>(strlen(songNumber)) * FONT_WIDTH;
        ssd1306_clear_square(pDisp, DISPLAY_WIDTH - width - 2, DISPLAY_HEIGHT + songNumberOffset, width + 2,
                             FONT_HEIGHT);
        ssd1306_draw_string(pDisp, DISPLAY_WIDTH - width, DISPLAY_HEIGHT + songNumberOffset, 1, songNumber);
        if (show) {
            songNumberOffset -= 0.5;
        }
        if (hide) {
            songNumberOffset += 0.5;
        }
    }


    void DanceFloor::draw3DPixelSphere(const int32_t x,
                                       const int32_t y,
                                       const float size,
                                       const int16_t xAxisRotation,
                                       const int16_t yAxisRotation) const {
        if (size < 1) {
            drawHorizonAwarePixel(x, y);
            return;
        }
        if (size <= 6) {
            drawFilledCircle(x, y, static_cast<int>(size));
            return;
        }
        float theta = 0;
        while (theta < 2 * M_PI) {
            float phi = 0;
            while (phi < M_PI) {
                // Latitude
                // Calculate 3D sphere coordinates
                float x3D = size * sin(phi) * cos(theta);
                float y3D = size * sin(phi) * sin(theta);
                float z3D = size * cos(phi);

                // Apply rotation around the X-axis
                const auto tempY = static_cast<float>(y3D * cos(xAxisRotation * M_PI / 180) - z3D
                                                      * sin(xAxisRotation * M_PI / 180));
                z3D = static_cast<float>(y3D * sin(xAxisRotation * M_PI / 180) + z3D * cos(xAxisRotation * M_PI / 180));
                y3D = tempY;

                // Apply rotation around the Y-axis
                x3D = static_cast<float>(x3D * cos(yAxisRotation * M_PI / 180) + z3D * sin(yAxisRotation * M_PI / 180));

                // Project 3D coordinates onto 2D plane
                drawHorizonAwarePixel(static_cast<int32_t>(static_cast<float>(x) + x3D),
                                      static_cast<int32_t>(static_cast<float>(y) + y3D));

                phi += 0.3;
            }
            theta += 0.3;
        }
    }

    void DanceFloor::drawSphereScene(const int totalY) {
        draw3DPixelSphere(sphereX, DISPLAY_Y_CENTER, sphereSize, sphereRotationX, sphereRotationY);
        sphereRotationY += 1 + std::min(90, totalY / 8);
        sphereRotationX -= 1;
        sphereSize += sphereSizeInc;
        sphereX += sphereXInc;
        if (sphereX < 0 || sphereX > DISPLAY_WIDTH) {
            sphereXInc = -sphereXInc;
        }

        if (sphereSize % 40 == 0) {
            sphereSizeInc += sphereSizeInc > 0 ? 1 : -1;
        }
        if (sphereSize > maxSphereSize) sphereSizeInc = -1;
        if (sphereSize < minSphereSize) sphereSizeInc = 1;
        if (sphereRotationX < 0) sphereRotationX = 359;
        if (sphereRotationY > 360) sphereRotationY -= 360;
    }

    void DanceFloor::drawScene(const kiss_fft_cpx *fft_out) {
        int totalY = 0;
        for (uint8_t x = LOW_FREQ_DOMINANCE_COMP_OFFSET; x < 127 + LOW_FREQ_DOMINANCE_COMP_OFFSET; x++) {
            const int i = static_cast<int>(1.8) * x;
            int y = static_cast<int>((fft_out[i].r + fft_out[i].i +
                                      fft_out[i + 1].r + fft_out[i + 1].i + static_cast<float>(i)) *
                                     compFactor);
            if (y > 0) {
                if (y > 28) y /= 8;
                totalY += y;

                if (!isOutsideOfRoundSpriteTimeWindow() && shouldUpdateRoundSprites()) {
                    if (x % rMod == 0 || x == 0 || x == 127) {
                        const RoundSprite roundSprite = {
                            .distance = 20, .x = static_cast<float>(random() % DISPLAY_WIDTH),
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
                if (shouldUpdateStarFieldSprites() && transition != FROM_ALTERNATIVE) {
                    if ((x % sMod == 0 || x == 0 || x == 127) && y > 0) {
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
            updateRoundSprites(transition == FROM_ALTERNATIVE);
        }
        if (shouldUpdateSoundSprites()) {
            drawFibonacciLandscape();
            if (transition != FROM_SPHERE) {
                drawStarrySky(false);
            }
            if (transition == NO_TRANSITION) {
                drawScroller();
            }
            updateSoundSprites();
        }
        if (shouldUpdateStarFieldSprites()) {
            updateStarFieldSprites();
            drawStarShip();
        }
        if (shouldUpdateSphere()) {
            drawSphereScene(totalY);
        }

        if (transition == FROM_ALTERNATIVE) {
            rMod = static_cast<int>(16 + (System::millis_now() - lastSceneChangeMS) / 96);
            sMod = static_cast<int>(64 + (System::millis_now() - lastSceneChangeMS) / 48);
        }

        auto millisSinceSongStart = SIDPlayer::millisSinceSongStart();
        if (SIDPlayer::getSongCount() > 1
            && millisSinceSongStart > SONG_NUMBER_DISPLAY_DELAY
            && millisSinceSongStart < SONG_NUMBER_DISPLAY_DURATION + SONG_NUMBER_DISPLAY_DELAY +
            SONG_NUMBER_SHOW_HIDE_DURATION * 2) {
            showCurrentSongNumber(millisSinceSongStart < SONG_NUMBER_SHOW_HIDE_DURATION + SONG_NUMBER_DISPLAY_DELAY,
                                  millisSinceSongStart > SONG_NUMBER_DISPLAY_DURATION + SONG_NUMBER_DISPLAY_DELAY +
                                  SONG_NUMBER_SHOW_HIDE_DURATION);
        }

        ssd1306_show(pDisp);

        if (!alternativeScene && !sphereScene && transition == NO_TRANSITION
            && System::millis_now() >= lastSceneChangeMS + SPECTRUM_SCENE_DURATION_MS) {
            transition = FROM_SPECTRUM;
            lastSceneChangeMS = System::millis_now();
        }
        if (alternativeScene && System::millis_now() >= lastSceneChangeMS + ALTERNATIVE_SCENE_DURATION) {
            lastSceneChangeMS = System::millis_now();
            transition = FROM_ALTERNATIVE;
        }
        if (sphereScene && System::millis_now() >= lastSceneChangeMS + END_SPHERE_SCENE_AFTER
            && sphereSize >= maxSphereSize / 2) {
            roundSpriteTargetXVelocity = 0;
            minSphereSize = 0;
            sphereSizeInc = -1;
            transition = FROM_SPHERE;
            horizon = 44;
            lastSceneChangeMS = System::millis_now();
        }
        if (transition == FROM_SPHERE && sphereSize <= minSphereSize) {
            roundSpriteXVelocity = 0;
            sphereScene = false;
            sphereSizeInc = 0;
            lastSceneChangeMS = System::millis_now();
        }
        if (transition == FROM_ALTERNATIVE
            && System::millis_now() >= lastSceneChangeMS + FROM_ALTERNATIVE_TRANSITION_DURATION) {
            transition = NO_TRANSITION;
            sphereScene = true;
            alternativeScene = false;
            starFieldVisible = false;
            lastSceneChangeMS = System::millis_now();
        }
        if (alternativeScene && !starFieldVisible && isWithinStarFieldTimeWindow()) {
            starFieldVisible = true;
            roundSpriteTargetXVelocity = -3;
        }
        if (transition == FROM_SPECTRUM) {
            horizon += 0.2;
            if (horizon >= 40) {
                transition = NO_TRANSITION;
                alternativeScene = true;
            }
        } else if (transition == FROM_SPHERE || transition == FROM_BEGIN) {
            horizon -= 0.2;
            if (horizon <= 10) {
                transition = NO_TRANSITION;
                alternativeScene = false;
                starFieldVisible = false;
                starShipX = -64;
                starShipY = DISPLAY_HEIGHT / 2 - 8;
                sphereSizeInc = 1;
                sphereSize = 100;
                minSphereSize = 6;
                rMod = 16;
                sMod = 64;
                showScroller = true;
                lastSceneChangeMS = System::millis_now();
            }
        }
    }


    void DanceFloor::randomizeExperience(char *experience) {
        const char *experiences[] = {"listening to", "enjoying", "experiencing", "feeling"};
        int randomIndex = random() % 4;
        strcpy(experience, experiences[randomIndex]);
    }

    void DanceFloor::visualize() {
        while (running) {
            if (!scrollerInitialized && strcmp(Catalog::getCurrentPlaylist()->getCurrentEntry()->fileName,
                                               SIDPlayer::getCurrentlyLoaded()->fileName) == 0) {
                SidInfo *entry = SIDPlayer::getSidInfo();
                randomizeExperience(experience);
                char extraText[50] = {};
                char name[44] = {};
                if (entry->songs > 1) {
                    sprintf(name, "%s (song %d)", entry->name, SIDPlayer::getCurrentSong() + 1);
                } else {
                    sprintf(name, "%s", entry->name);
                }
                if (entry->sidChipBase3) {
                    sprintf(extraText, "Did you know that this song uses three SID chips?");
                } else if (entry->sidChipBase2) {
                    sprintf(extraText, "Fun fact: This song uses two SID chips!");
                }
                snprintf(scrollText, sizeof(scrollText),
                         "This is %s by %s (%s) and you are %s %s on a SIDPod. %s",
                         name, entry->author, entry->released, experience,
                         entry->isPSID ? "it" : "this RSID",
                         extraText);
                scrollerInitialized = true;
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

    void DanceFloor::init() {
        rsOffset = DISPLAY_WIDTH + 32;
        rvOffset = 0;
        showScroller = false;
        for (auto &soundSprite: soundSprites) {
            soundSprite = {};
        }
        for (auto &roundSprite: roundSprites) {
            roundSprite = {};
        }
        for (auto &startFieldSprite: starFieldSprites) {
            startFieldSprite = {};
        }

        alternativeScene = false;
        starFieldVisible = false;
        sphereScene = false;
        starShipX = -64;
        starShipY = DISPLAY_Y_CENTER - 8;
        letStarShipRoam = false;
        transition = FROM_BEGIN;
        horizon = 40;
        songNumberOffset = 0;
        sphereRotationX = 0;
        sphereRotationY = 0;
        sphereSize = 100;
        sphereSizeInc = 1;
        sphereX = DISPLAY_X_CENTER;
        sphereXInc = 1;
        maxSphereSize = 64;
        roundSpriteXVelocity = 0;
        roundSpriteTargetXVelocity = 0;
        scrollerInitialized = false;
        showScroller = true;
        lastSceneChangeMS = System::millis_now();
    }

    void DanceFloor::start() {
        for (int i = 0; i < SIDPLAYER_STARTUP_GRACE_TIME; i++) {
            if (SIDPlayer::isPlaying()) break;
            busy_wait_ms(1);
        }
        running = true;
        freeze = false;
        visualize();
    }

    void DanceFloor::stop() {
        running = false;
    }
}
