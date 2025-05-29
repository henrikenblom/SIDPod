//
// Created by Henrik Enblom on 2025-05-27.
//

#ifndef GL_H
#define GL_H
#include "platform_config.h"
#include "ssd1306.h"


class GL {
public:
    explicit GL(ssd1306_t *pDisp) : pDisp(pDisp) {
        i2c_init(DISPLAY_I2C_BLOCK, I2C_BAUDRATE);
        gpio_set_function(DISPLAY_GPIO_BASE_PIN, GPIO_FUNC_I2C);
        gpio_set_function(DISPLAY_GPIO_BASE_PIN + 1, GPIO_FUNC_I2C);
        gpio_pull_up(DISPLAY_GPIO_BASE_PIN);
        gpio_pull_up(DISPLAY_GPIO_BASE_PIN + 1);
        pDisp->external_vcc = DISPLAY_EXTERNAL_VCC;
        ssd1306_init(pDisp, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_I2C_ADDRESS, i2c1);
        cursorIntervalCounter = 0;
    }

    void drawPixel(int32_t x, int32_t y, int32_t clip = 0) const;

    void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const;

    void clearSquare(int32_t x1, int32_t y1, int32_t width, int32_t height) const;

    void drawEmptySquare(int32_t x1, int32_t y1, int32_t width, int32_t height) const;

    void drawString(int32_t x, int32_t y, const char *pStr) const;

    void showBMPImage(const uint8_t *data, long size) const;

    void showRawImage(uint8_t width,
                      uint8_t height,
                      const uint8_t *data,
                      int32_t xOffset,
                      int32_t yOffset,
                      bool frame = true) const;

    void drawDottedHorizontalLine(uint8_t y);

    void drawHorizontalLine(uint8_t y) const;

    void drawCircle(int32_t x, int32_t y, int32_t radius, int32_t clip = 0) const;

    void drawFilledCircle(int32_t x, int32_t y, int32_t radius, int32_t clip = 0) const;

    void draw3DPixelSphere(int32_t x,
                           int32_t y,
                           float size,
                           int16_t xAxisRotation,
                           int16_t yAxisRotation,
                           int32_t clip = 0) const;

    void drawHeader(const char *title);

    void drawInput(const char *label, const char *text, int8_t maxLength);

    void drawDialog(const char *text) const;

    void drawOpenSymbol(int32_t y) const;

    void drawNowPlayingSymbol(int32_t y, bool animate = true);

    void crossOutLine(int32_t y) const;

    void animateLongText(const char *title, int32_t y, int32_t xMargin, float *offsetCounter) const;

    void clear() const;

    void update() const;

    void displayOn() const;

    void displayOff() const;

private:
    ssd1306_t *pDisp;
    uint8_t horizontalLineDitherOffset = 0;
    int32_t cursorIntervalCounter;
    float longTitleScrollOffset{}, headerScrollOffset{}, playingSymbolAnimationCounter = 0;

    bool showCursor();
};


#endif //GL_H
