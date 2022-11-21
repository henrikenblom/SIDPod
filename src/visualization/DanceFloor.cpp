//
// Created by Henrik Enblom on 2022-11-21.
//

#include <random>
#include "DanceFloor.h"
#include "../platform_config.h"
#include "kiss_fftr.h"
#include "../audio/SIDPlayer.h"

int sprite_index = 0;
char scrollText[160];
int16_t scrollLimit = -2048;
uint8_t horizontalLineDitherOffset = 0;
int rvOffset = 0;
int rsOffset = DISPLAY_WIDTH + 32;
uint32_t delay = 5;
bool running = false;
kiss_fftr_cfg fft_cfg;
double factor = 0.000000004;
DanceFloor::SoundSprite soundSprites[SOUND_SPRITE_COUNT];
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
uint16_t fibonacci[FIBONACCI_NUMBER_COUNT];
kiss_fft_scalar fftIn[FFT_SAMPLES];
kiss_fft_cpx fftOut[FFT_SAMPLES];
ssd1306_t *pDisp;

void DanceFloor::initFibonacci() {
    fibonacci[0] = 0;
    fibonacci[1] = 1;
    for (int i = 2; i < FIBONACCI_NUMBER_COUNT; i++) {
        fibonacci[i] = fibonacci[i - 1] + fibonacci[i - 2];
    }
}

void DanceFloor::drawHorizontalLine(uint8_t y) {
    if (y <= DISPLAY_HEIGHT) {
        for (int x = 0; x < 127; x += 2) {
            ssd1306_draw_pixel(pDisp, x + horizontalLineDitherOffset, y);
        }
    }
    horizontalLineDitherOffset ^= 1;
}

void DanceFloor::drawScroller() {
    ssd1306_draw_string(pDisp, rsOffset--, 1, 1, scrollText);
    if (rsOffset < scrollLimit) rsOffset = DISPLAY_WIDTH + 1;
}

void DanceFloor::drawStarrySky() {
    for (auto &sprite: starSprites) {
        if (random() % (100) != 1) {
            ssd1306_draw_pixel(pDisp, sprite.x, sprite.y);
        }
    }
}

void DanceFloor::drawFibonacciLandscape() {
    drawHorizontalLine(10);
    for (int i = 0; i < 16; i++) {
        drawHorizontalLine(10 + (fibonacci[i] / (20 - rvOffset)));
    }
    ssd1306_draw_line(pDisp, 96, 10, 127, 23);
    ssd1306_draw_line(pDisp, 0, 23, 32, 10);
    ssd1306_draw_line(pDisp, 68, 10, 80, 31);
    ssd1306_draw_line(pDisp, 39, 40, 60, 10);
    if (rvOffset++ > 4) rvOffset = 0;
}

void DanceFloor::drawSoundSprite(DanceFloor::SoundSprite sprite) {
    int x;
    int y1 = 39 - sprite.distance - sprite.velocity - 10;
    int y2 = y1 + sprite.velocity;
    if (sprite.frequency_bin <= 64) {
        x = sprite.frequency_bin + (fibonacci[sprite.distance] / 4);
    } else {
        x = sprite.frequency_bin - (fibonacci[sprite.distance] / 4);
    }
    ssd1306_draw_line(pDisp, x, std::max(0, y1), x, std::min(31, y2));
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

void DanceFloor::drawScene(kiss_fft_cpx *fft_out) {
    ssd1306_clear(pDisp);
    drawFibonacciLandscape();
    drawStarrySky();
    drawScroller();
    for (int x = 0; x < 127; x++) {
        int i = (int) (1.6 * (float) x);
        int y = (int) ((fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i +
                        fft_out[i + 1].r * fft_out[i + 1].r + fft_out[i + 1].i * fft_out[i + 1].i) *
                       factor);
        if (y > 0) {
            if (y > 32) y /= 7;
            SoundSprite sprite = {.velocity = std::min(16, y), .distance = 20, .frequency_bin = x};
            soundSprites[sprite_index++] = sprite;
            if (sprite_index > SOUND_SPRITE_COUNT) sprite_index = 0;
        }
    }
    updateSoundSprites();
    ssd1306_show(pDisp);
}

void DanceFloor::visualize() {
    while (running) {
        //multicore_fifo_pop_blocking();
        for (int offset = 0; offset < SAMPLES_PER_BUFFER; offset += FFT_SAMPLES * 2) {
            sleep_ms(delay);
            uint64_t sum = 0;
            for (int i = offset; i < offset + (FFT_SAMPLES * 2); i += 2) {
                sum += intermediateBuffer[i] >> 2;
            }
            float avg = (float) sum / FFT_SAMPLES;
            int j = 0;
            for (int i = offset; i < offset + (FFT_SAMPLES * 2); i += 2) {
                fftIn[j++] = (float) (intermediateBuffer[i] >> 2) - avg;
            }

            kiss_fftr(fft_cfg, fftIn, fftOut);

            drawScene(fftOut);
        }
    }
}

void DanceFloor::init(ssd1306_t *_pDisp) {
    pDisp = _pDisp;
    initFibonacci();
    fft_cfg = kiss_fftr_alloc(FFT_SAMPLES, false, nullptr, nullptr);
}

void DanceFloor::start(PSIDCatalogEntry *entry) {
    snprintf(scrollText, sizeof(scrollText), "This is %s by %s (%s) and you are experiencing it on a SIDPod.", entry->title, entry->author, entry->released);
    rvOffset = 0;
    rsOffset = DISPLAY_WIDTH + 32;
    running = true;
    visualize();
}

void DanceFloor::stop() {
    running = false;
}
