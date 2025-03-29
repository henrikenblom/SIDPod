#include "DanceFloor.h"
#include "../platform_config.h"
#include "kiss_fftr.h"
#include "../audio/SIDPlayer.h"
#include "../audio/C64.h"

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

    void DanceFloor::drawHorizontalLine(uint8_t y) {
        if (y <= DISPLAY_HEIGHT) {
            for (int x = 0; x < DISPLAY_WIDTH - 1; x += 2) {
                ssd1306_draw_pixel(pDisp, x + horizontalLineDitherOffset, y);
            }
        }
        horizontalLineDitherOffset ^= 1;
    }

    void DanceFloor::drawScroller() {
        if (showScroller) {
            ssd1306_draw_string(pDisp, rsOffset--, 1, 1, scrollText);
            if (rsOffset < scrollLimit) rsOffset = DISPLAY_WIDTH + 1;
        }
    }

    void DanceFloor::drawStarrySky() {
        for (auto &sprite: starSprites) {
            if (random() % (100) != 1) {
                ssd1306_draw_pixel(pDisp, sprite.x, sprite.y);
            }
        }
    }

    void DanceFloor::drawCircle(const int32_t x, const int32_t y, const int32_t radius) const {
        int32_t f = 1 - radius;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * radius;
        int32_t xi = 0;
        int32_t yi = radius;

        ssd1306_draw_pixel(pDisp, x, y + radius);
        ssd1306_draw_pixel(pDisp, x, y - radius);
        ssd1306_draw_pixel(pDisp, x + radius, y);
        ssd1306_draw_pixel(pDisp, x - radius, y);

        while (xi < yi) {
            if (f >= 0) {
                yi--;
                ddF_y += 2;
                f += ddF_y;
            }
            xi++;
            ddF_x += 2;
            f += ddF_x;

            ssd1306_draw_pixel(pDisp, x + xi, y + yi);
            ssd1306_draw_pixel(pDisp, x - xi, y + yi);
            ssd1306_draw_pixel(pDisp, x + xi, y - yi);
            ssd1306_draw_pixel(pDisp, x - xi, y - yi);
            ssd1306_draw_pixel(pDisp, x + yi, y + xi);
            ssd1306_draw_pixel(pDisp, x - yi, y + xi);
            ssd1306_draw_pixel(pDisp, x + yi, y - xi);
            ssd1306_draw_pixel(pDisp, x - yi, y - xi);
        }
    }

    void DanceFloor::drawFibonacciLandscape() {
        drawHorizontalLine(10);
        for (const unsigned short i: fibonacci) {
            drawHorizontalLine(10 + i / (20 - rvOffset));
        }
        ssd1306_draw_line(pDisp, 0, 25, 23, 10);
        ssd1306_draw_line(pDisp, 42, 31, 48, 10);
        ssd1306_draw_line(pDisp, 79, 10, 86, 31);
        ssd1306_draw_line(pDisp, 104, 10, 127, 25);
        if (rvOffset++ > 4) rvOffset = 0;
    }

    void DanceFloor::drawSoundSprite(SoundSprite sprite) const {
        int x;
        double f = 0.012;
        int y1 = 39 - sprite.distance - sprite.velocity - 10;
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
            if (sprite.distance > -16) {
                drawCircle(sprite.x, sprite.y, sprite.distance / 2);
                sprite.distance -= 4;
            }
            roundSprite = sprite;
        }
    }


    void DanceFloor::drawPausedLabel() {
        int labelWidth = (int) strlen(pausedLabel) * FONT_WIDTH;
        int windowWidth = labelWidth + 4;
        int windowHeight = FONT_HEIGHT + 1;
        ssd1306_clear_square(pDisp, displayCenter - (windowWidth / 2), 4, windowWidth, windowHeight);
        ssd13606_draw_empty_square(pDisp, displayCenter - (windowWidth / 2), 1, windowWidth, windowHeight + 4);
        ssd1306_draw_string(pDisp, displayCenter - (labelWidth / 2) + 1, 5, 1, pausedLabel);
    }

    void DanceFloor::drawScene(kiss_fft_cpx *fft_out) {
        for (uint8_t x = 0; x < 127; x++) {
            const int i = static_cast<int>(1.6 * x);
            int y = static_cast<int>((fft_out[i].r + fft_out[i].i +
                                      fft_out[i + 1].r + fft_out[i + 1].i) *
                                     compFactor);
            if (y > 0) {
                if (y > 28) y /= 8;

                if (alternativeScene) {
                    if (x % 16 == 0 || x == 0) {
                        RoundSprite roundSprite = {
                            .velocity = static_cast<int8_t>(std::min(16,
                                                                     y)),
                            .distance = 20, .x = random() % DISPLAY_WIDTH - x,
                            .y = random() % DISPLAY_HEIGHT
                        };
                        roundSprites[roundSpriteIndex++] = roundSprite;
                        if (roundSpriteIndex > ROUND_SPRITE_COUNT) roundSpriteIndex = 0;
                    }
                } else {
                    SoundSprite sprite = {
                        .velocity = static_cast<int8_t>(std::min(16,
                                                                 y)),
                        .distance = 20, .frequency_bin = x
                    };
                    soundSprites[sprite_index++] = sprite;
                    if (sprite_index > SOUND_SPRITE_COUNT) sprite_index = 0;
                }
            }
        }

        ssd1306_clear(pDisp);
        if (alternativeScene) {
            updateRoundSprites();
        } else {
            drawFibonacciLandscape();
            drawStarrySky();
            drawScroller();
            updateSoundSprites();
        }
        ssd1306_show(pDisp);

        if (millis_now() >= millisSinceLastSceneChange + (alternativeScene ? 15000 : 60000)) {
            millisSinceLastSceneChange = millis_now();
            alternativeScene = !alternativeScene;
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
                sid_info *entry = SIDPlayer::getSidInfo();
                randomizeExperience(experience);
                snprintf(scrollText, sizeof(scrollText),
                         "This is %s by %s (%s) and you are %s %s on a SIDPod v2. Codename \"Residious\".",
                         entry->title, entry->author, entry->released, experience,
                         entry->rsid == true ? "this RSID" : "it");
                showScroller = true;
            }
            compFactor = SIDPlayer::lineLevelOn() ? LINE_LEVEL_SPECTRUM_COMPENSATION : DEFAULT_SPECTRUM_COMPENSATION;
            if (SIDPlayer::isPlaying()) {
                freeze = false;
                int j = 0;
                for (int i = 0; i < FFT_SAMPLES; i += 1) {
                    fftIn[j++] = intermediateBuffer[i];
                }
                kiss_fftr(fft_cfg, fftIn, fftOut);

                drawScene(fftOut);
            } else if (!SIDPlayer::loadingWasSuccessful()) {
                stop();
            } else if (!freeze) {
                ssd1306_clear(pDisp);
                drawFibonacciLandscape();
                drawStarrySky();
                drawPausedLabel();
                ssd1306_show(pDisp);
                freeze = true;
            }
        }
    }

    void DanceFloor::start(CatalogEntry *_selectedEntry) {
        selectedEntry = _selectedEntry;
        rvOffset = 0;
        rsOffset = DISPLAY_WIDTH + 32;
        running = true;
        freeze = false;
        showScroller = false;
        for (int i = 0; i < SIDPLAYER_STARTUP_GRACE_TIME; i++) {
            if (SIDPlayer::isPlaying()) break;
            busy_wait_ms(1);
        }
        visualize();
    }

    void DanceFloor::stop() {
        running = false;
    }
}
