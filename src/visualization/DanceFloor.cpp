#include "DanceFloor.h"
#include "../platform_config.h"
#include "kiss_fftr.h"
#include "../audio/SIDPlayer.h"
#include "sid.h"

int sprite_index = 0;
char scrollText[160];
char pausedLabel[7] = "PAUSED";
int16_t scrollLimit = -2048;
uint8_t horizontalLineDitherOffset = 0;
int rvOffset = 0;
int rsOffset = DISPLAY_WIDTH + 32;
bool running = false;
bool freeze = false;
bool showScroller = false;
kiss_fftr_cfg fft_cfg;
double compFactor = DEFAULT_SPECTRUM_COMPENSATION;
CatalogEntry *selectedEntry;

void (*stopCallback)() = nullptr;

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

void DanceFloor::drawFibonacciLandscape() {
    drawHorizontalLine(10);
    for (int i = 0; i < 16; i++) {
        drawHorizontalLine(10 + (fibonacci[i] / (20 - rvOffset)));
    }
    ssd1306_draw_line(pDisp, 0, 25, 23, 10); // Leftmost
    ssd1306_draw_line(pDisp, 42, 31, 48, 10);
    ssd1306_draw_line(pDisp, 79, 10, 86, 31);
    ssd1306_draw_line(pDisp, 104, 10, 127, 25); // Rightmost
    if (rvOffset++ > 4) rvOffset = 0;
}

void DanceFloor::drawSoundSprite(DanceFloor::SoundSprite sprite) {
    int x;
    double f = 0.012;
    int y1 = 39 - sprite.distance - sprite.velocity - 10;
    int y2 = sprite.distance < 7 ? (int) (y1 + sprite.velocity * 0.4)
                                 : (int) (y1 + sprite.velocity + (sprite.distance * 0.29));
    int perspectiveComp;
    if (sprite.frequency_bin <= 64) {
        perspectiveComp = (int) ((64 - sprite.frequency_bin) * (sprite.distance * f));
        x = sprite.frequency_bin + perspectiveComp;
    } else {
        perspectiveComp = (int) ((sprite.frequency_bin - 64) * (sprite.distance * f));
        x = sprite.frequency_bin - perspectiveComp;
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


void DanceFloor::drawPausedLabel() {
    ssd1306_clear_square(pDisp, (DISPLAY_WIDTH / 2) - 26, 4, 52, 9);
    ssd13606_draw_empty_square(pDisp, (DISPLAY_WIDTH / 2) - 26, 4 - 3, 52, 13);
    ssd1306_draw_string(pDisp, (DISPLAY_WIDTH / 2) - 22, 5, 1, pausedLabel);
}

void DanceFloor::drawScene(kiss_fft_cpx *fft_out) {
    ssd1306_clear(pDisp);
    drawFibonacciLandscape();
    drawStarrySky();
    drawScroller();
    for (uint8_t x = 0; x < 127; x++) {
        int i = (int) (1.6 * (float) x);
        int y = (int) ((fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i +
                        fft_out[i + 1].r * fft_out[i + 1].r + fft_out[i + 1].i * fft_out[i + 1].i) *
                       compFactor);
        if (y > 0) {
            if (y > 28) y /= 8;
            SoundSprite sprite = {.velocity = static_cast<int8_t>(std::min(16, y)), .distance = 20, .frequency_bin = x};
            soundSprites[sprite_index++] = sprite;
            if (sprite_index > SOUND_SPRITE_COUNT) sprite_index = 0;
        }
    }
    updateSoundSprites();
    ssd1306_show(pDisp);
}

void DanceFloor::visualize() {
    while (running) {
        if (!showScroller && strcmp(selectedEntry->fileName, SIDPlayer::getCurrentlyLoaded()->fileName) == 0) {
            sid_info *entry = SIDPlayer::getSidInfo();
            snprintf(scrollText, sizeof(scrollText), "This is %s by %s (%s) and you are experiencing it on a SIDPod.",
                     entry->title, entry->author, entry->released);
            showScroller = true;
        }
        compFactor = SIDPlayer::lineLevelOn() ? LINE_LEVEL_SPECTRUM_COMPENSATION : DEFAULT_SPECTRUM_COMPENSATION;
        if (SIDPlayer::isPlaying()) {
            freeze = false;
            for (int offset = 0; offset < SAMPLES_PER_BUFFER; offset += FFT_SAMPLES * 2) {
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
    if (stopCallback != nullptr) {
        (*stopCallback)();
        stopCallback = nullptr;
    }
}

void DanceFloor::init(ssd1306_t *_pDisp) {
    pDisp = _pDisp;
    initFibonacci();
    fft_cfg = kiss_fftr_alloc(FFT_SAMPLES, false, nullptr, nullptr);
}

void DanceFloor::start(CatalogEntry *_selectedEntry) {
    selectedEntry = _selectedEntry;
    rvOffset = 0;
    rsOffset = DISPLAY_WIDTH + 32;
    running = true;
    freeze = false;
    showScroller = false;
    visualize();
}

void DanceFloor::stop() {
    running = false;
}

void DanceFloor::stopWithCallback(void (*callback)()) {
    stopCallback = callback;
    running = false;
}
