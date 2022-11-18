//
// Created by Henrik Enblom on 2022-11-15.
//

#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <cstdio>
#include "UI.h"
#include "platform_config.h"
#include "ssd1306.h"
#include "PSIDCatalog.h"
#include "audio/SIDPlayer.h"

ssd1306_t disp;
bool active = false;
bool lastButtonState = false;

void UI::initUI() {
    i2c_init(i2c1, I2C_BAUDRATE);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);
    disp.external_vcc = false;
    ssd1306_init(&disp, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_I2C_ADDRESS, i2c1);
    ssd1306_poweroff(&disp);
}

void UI::showUI() {
    if (active) {
        showSongSelector();
    } else {
        showRasterBars();
    }
}


void UI::showSongSelector() {
    ssd1306_clear(&disp);
    PSIDCatalog::getWindow();
    uint8_t y = 0;
    for (auto entry: PSIDCatalog::getWindow()) {
        if (entry.selected) {
            ssd1306_draw_pixel(&disp, 0, y + 2);
            ssd1306_draw_line(&disp, 0, y + 3, 2, y + 3);
            ssd1306_draw_pixel(&disp, 0, y + 4);
        }
        ssd1306_draw_string(&disp, 4, y, 1, entry.title);
        y += 8;
    }
    ssd1306_show(&disp);
    checkButtonPushed();
}

void UI::encoderCallback(uint gpio, __attribute__((unused)) uint32_t events) {
    uint32_t gpio_state = (gpio_get_all() >> 10) & 0b0111;

    static bool ccw_fall = false;
    static bool cw_fall = false;

    uint8_t enc_value = (gpio_state & 0x03);

    if (gpio == ENC_A) {
        if ((!cw_fall) && (enc_value == 0b10))
            cw_fall = true;

        if ((ccw_fall) && (enc_value ==
                           0b00)) {
            cw_fall = false;
            ccw_fall = false;
            PSIDCatalog::selectPrevious();
        }
    }

    if (gpio == ENC_B) {
        if ((!ccw_fall) && (enc_value == 0b01))
            ccw_fall = true;

        if ((cw_fall) && (enc_value == 0b00)) {
            cw_fall = false;
            ccw_fall = false;
            PSIDCatalog::selectNext();
        }

    }
}

void UI::stop() {
    active = false;
}

void UI::start() {
    active = true;
    gpio_init(ENC_SW);                    //Initialise a GPIO for (enabled I/O and set func to GPIO_FUNC_SIO)
    gpio_set_dir(ENC_SW, GPIO_IN);
    gpio_pull_up(ENC_SW);

    gpio_init(ENC_A);
    gpio_set_dir(ENC_A, GPIO_IN);
    gpio_disable_pulls(ENC_A);

    gpio_init(ENC_B);
    gpio_set_dir(ENC_B, GPIO_IN);
    gpio_disable_pulls(ENC_B);

    gpio_set_irq_enabled_with_callback(ENC_B, GPIO_IRQ_EDGE_FALL, true, &encoderCallback);
    gpio_set_irq_enabled(ENC_A, GPIO_IRQ_EDGE_FALL, true);

    ssd1306_poweron(&disp);
}

inline void UI::showRasterBars() {
    ssd1306_clear(&disp);
    int y = rand() % (32);
    ssd1306_draw_line(&disp, 0, y, 127, y);
    unacked_ssd1306_show(&disp);
}

void UI::checkButtonPushed() {
    bool currentState = !gpio_get(ENC_SW);
    if (currentState && currentState != lastButtonState) {
        SIDPlayer::play();
    }
    lastButtonState = currentState;
}
