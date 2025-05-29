//
// Created by Henrik Enblom on 2025-05-27.
//

#include "GL.h"

#include <cstdio>
#include <cstring>
#include <math.h>

#include "Buddy.h"
#include "platform_config.h"
#include "System.h"

void GL::drawPixel(int32_t x, int32_t y, int32_t clip) const {
    if (clip) {
        if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT && y < clip) {
            ssd1306_draw_pixel(pDisp, x, y);
        }
    } else {
        ssd1306_draw_pixel(pDisp, x, y);
    }
}

void GL::drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const {
    ssd1306_draw_line(pDisp, x1, y1, x2, y2);
}

void GL::clearSquare(int32_t x1, int32_t y1, int32_t width, int32_t height) const {
    ssd1306_clear_square(pDisp, x1, y1, width, height);
}

void GL::drawEmptySquare(int32_t x1, int32_t y1, int32_t width, int32_t height) const {
    ssd13606_draw_empty_square(pDisp, x1, y1, width, height);
}

void GL::drawString(int32_t x, int32_t y, const char *pStr) const {
    ssd1306_draw_string(pDisp, x, y, 1, pStr);
}

void GL::showBMPImage(const uint8_t *data, const long size) const {
    ssd1306_bmp_show_image(pDisp, data, size);
}

void GL::showRawImage(uint8_t width,
                      uint8_t height,
                      const uint8_t *data,
                      int32_t xOffset,
                      int32_t yOffset,
                      bool frame) const {
    clearSquare(xOffset - 1, yOffset - 1, width + 2, height + 2);
    int32_t x = 0;
    int32_t y = 0;
    for (int i = 0; i < width * height / 8; i++) {
        uint8_t byte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (IS_BIT_SET(byte, bit)) {
                drawPixel(x + xOffset, y + yOffset);
            }
            if (x++ == width - 1) {
                x = 0;
                y++;
            }
        }
    }
    if (frame) {
        drawEmptySquare(xOffset - 1, yOffset - 1, width + 2, height + 2);
    }
}

void GL::drawDottedHorizontalLine(const uint8_t y) {
    if (y <= DISPLAY_HEIGHT) {
        for (int x = 0; x < DISPLAY_WIDTH - 1; x += 2) {
            ssd1306_draw_pixel(pDisp, x + horizontalLineDitherOffset, y);
        }
    }
    horizontalLineDitherOffset ^= 1;
}

void GL::drawHorizontalLine(const uint8_t y) const {
    if (y <= DISPLAY_HEIGHT) {
        ssd1306_draw_line(pDisp, 0, y, DISPLAY_WIDTH - 1, y);
    }
}


void GL::drawCircle(int32_t x, int32_t y, int32_t radius, int32_t clip) const {
    int32_t f = 1 - radius;
    int32_t ddF_x = 1;
    int32_t ddF_y = -2 * radius;
    int32_t xi = 0;
    int32_t yi = radius;

    drawPixel(x, y + radius, clip);
    drawPixel(x, y - radius, clip);
    drawPixel(x + radius, y, clip);
    drawPixel(x - radius, y, clip);

    while (xi < yi) {
        if (f >= 0) {
            yi--;
            ddF_y += 2;
            f += ddF_y;
        }
        xi++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x + xi, y + yi, clip);
        drawPixel(x - xi, y + yi, clip);
        drawPixel(x + xi, y - yi, clip);
        drawPixel(x - xi, y - yi, clip);
        drawPixel(x + yi, y + xi, clip);
        drawPixel(x - yi, y + xi, clip);
        drawPixel(x + yi, y - xi, clip);
        drawPixel(x - yi, y - xi, clip);
    }
}

void GL::drawFilledCircle(int32_t x, int32_t y, int32_t radius, int32_t clip) const {
    for (int32_t i = -radius; i <= radius; i++) {
        for (int32_t j = -radius; j <= radius; j++) {
            if (i * i + j * j <= radius * radius) {
                drawPixel(x + i, y + j, clip);
            }
        }
    }
}

void GL::draw3DPixelSphere(int32_t x,
                           int32_t y,
                           float size,
                           int16_t xAxisRotation,
                           int16_t yAxisRotation,
                           int32_t clip) const {
    if (size < 1) {
        drawPixel(x, y, clip);
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
            drawPixel(static_cast<int32_t>(static_cast<float>(x) + x3D),
                      static_cast<int32_t>(static_cast<float>(y) + y3D), clip);

            phi += 0.3;
        }
        theta += 0.3;
    }
}

void GL::drawHeader(const char *title) {
    if (const size_t stringWidth = strlen(title) * FONT_WIDTH; stringWidth > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
        this->animateLongText(title, 0, 12, &headerScrollOffset);
        this->drawLine(0, 0, 8, 0);
        this->drawLine(0, 2, 8, 2);
        this->drawLine(0, 4, 8, 4);
        this->drawLine(0, 6, 8, 6);
    } else {
        this->drawLine(0, 0, DISPLAY_WIDTH, 0);
        this->drawLine(0, 2, DISPLAY_WIDTH, 2);
        this->drawLine(0, 4, DISPLAY_WIDTH, 4);
        this->drawLine(0, 6, DISPLAY_WIDTH, 6);
        this->clearSquare(8, 0, static_cast<int>(stringWidth + FONT_WIDTH), FONT_HEIGHT);
        this->drawString(12, 0, title);
    }
}

void GL::drawInput(const char *label, const char *text, int8_t maxLength) {
    const int textLength = static_cast<int>(strlen(text));
    const int textWidth = textLength * FONT_WIDTH;
    const int labelWidth = static_cast<int>(strlen(label) * FONT_WIDTH);
    this->drawLine(0, 0, 8, 0);
    this->drawLine(0, 2, 8, 2);
    this->drawLine(0, 4, 8, 4);
    this->drawLine(0, 6, 8, 6);
    this->drawString(12, 0, label);
    this->drawString(12 + labelWidth, 0, text);
    if (textLength < maxLength && showCursor()) {
        this->drawString(12 + labelWidth + textWidth, 2, "_");
    }
}

void GL::drawDialog(const char *text) const {
    const int labelWidth = static_cast<int>(strlen(text)) * FONT_WIDTH;
    const int windowWidth = labelWidth + 4;
    constexpr int windowHeight = FONT_HEIGHT + 1;
    this->clearSquare(DISPLAY_X_CENTER - (windowWidth / 2), DISPLAY_Y_CENTER - (windowHeight / 2) - 1,
                      windowWidth, windowHeight + 1);
    this->drawEmptySquare(DISPLAY_X_CENTER - (windowWidth / 2), DISPLAY_Y_CENTER - (windowHeight / 2) - 1,
                          windowWidth, windowHeight + 1);
    this->drawString(DISPLAY_X_CENTER - (labelWidth / 2) + 1, DISPLAY_Y_CENTER - FONT_HEIGHT / 2 + 1, text);
}

void GL::drawOpenSymbol(int32_t y) const {
    this->drawPixel(0, y + 2);
    this->drawLine(0, y + 3, 2, y + 3);
    this->drawPixel(0, y + 4);
}

void GL::drawNowPlayingSymbol(const int32_t y, const bool animate) {
    int bar1 = 4;
    int bar2 = 1;
    int bar3 = 5;
    int bar4 = 3;
    if (animate) {
        bar1 = static_cast<int>(playingSymbolAnimationCounter);
        bar2 = random() % (NOW_PLAYING_SYMBOL_HEIGHT);
        bar3 = static_cast<int>(sin(playingSymbolAnimationCounter) * (NOW_PLAYING_SYMBOL_HEIGHT - 2))
               + NOW_PLAYING_SYMBOL_HEIGHT - 3;
        bar4 = static_cast<int>((NOW_PLAYING_SYMBOL_HEIGHT - bar1) * 0.5 + bar2 * 0.5);
        if ((playingSymbolAnimationCounter += NOW_PLAYING_SYMBOL_ANIMATION_SPEED) >
            NOW_PLAYING_SYMBOL_HEIGHT)
            playingSymbolAnimationCounter = 0;
    }
    this->drawLine(0, y + NOW_PLAYING_SYMBOL_HEIGHT - bar1, 0, y + NOW_PLAYING_SYMBOL_HEIGHT);
    this->drawLine(1, y + NOW_PLAYING_SYMBOL_HEIGHT - bar2, 1, y + NOW_PLAYING_SYMBOL_HEIGHT);
    this->drawLine(2, y + NOW_PLAYING_SYMBOL_HEIGHT - bar3, 2, y + NOW_PLAYING_SYMBOL_HEIGHT);
    this->drawLine(3, y + NOW_PLAYING_SYMBOL_HEIGHT - bar4, 3, y + NOW_PLAYING_SYMBOL_HEIGHT);
}

void GL::crossOutLine(int32_t y) const {
    this->drawLine(SONG_LIST_LEFT_MARGIN, y + 3, DISPLAY_WIDTH, y + 3);
}

void GL::animateLongText(const char *title, int32_t y, int32_t xMargin, float *offsetCounter) const {
    this->drawString(xMargin - static_cast<int32_t>(*offsetCounter), y, title);
    int scrollRange = (int) strlen(title) * FONT_WIDTH - DISPLAY_WIDTH + xMargin;
    float advancement =
            *offsetCounter > 1 && static_cast<int>(*offsetCounter) < scrollRange
                ? 1
                : 0.04;
    if (static_cast<int>(*offsetCounter += advancement) > scrollRange)
        *offsetCounter = 0;
    this->clearSquare(0, y, xMargin - 1, y + FONT_HEIGHT);
}

void GL::clear() const {
    ssd1306_clear(pDisp);
}

void GL::update() const {
#if USE_BUDDY
    const auto buddy = Buddy::getInstance();
    if (buddy->scribbleDataIsFresh()) {
        showRawImage(28, 28, buddy->scribbleBuffer, DISPLAY_WIDTH - 30, 2, true);
    }
#endif
    System::virtualVBLSync();
    ssd1306_show(pDisp);
}

void GL::displayOn() const {
    ssd1306_poweron(pDisp);
    busy_wait_ms(DISPLAY_STATE_CHANGE_DELAY_MS);
}

void GL::displayOff() const {
    this->clear();
    this->update();
    ssd1306_poweroff(pDisp);
}

bool GL::showCursor() {
    if (cursorIntervalCounter++ > CURSOR_BLINK_INTERVAL_VBL * 2) {
        cursorIntervalCounter = 0; // Reset counter
    }
    return cursorIntervalCounter < CURSOR_BLINK_INTERVAL_VBL;
}
