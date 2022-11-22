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
#include "quadrature_encoder.pio.h"
#include "visualization/DanceFloor.h"
#include "sidpod_bmp.h"

ssd1306_t disp;
bool active = false;
bool lastButtonState = false;
bool visualize = false;
int encNewValue, encDelta, encOldValue = 0;
struct repeating_timer userControlTimer{};

void UI::initUI() {
    i2c_init(i2c1, I2C_BAUDRATE);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);
    gpio_init(ENC_SW);
    gpio_set_dir(ENC_SW, GPIO_IN);
    gpio_pull_up(ENC_SW);
    uint offset = pio_add_program(pio1, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio1, 1, offset, ENC_A, 0);
    disp.external_vcc = false;
    ssd1306_init(&disp, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_I2C_ADDRESS, i2c1);
    DanceFloor::init(&disp);
}

void UI::screenOn() {
    ssd1306_clear(&disp);
    ssd1306_show(&disp);
    ssd1306_poweron(&disp);
}

void UI::showSplash() {
    ssd1306_clear(&disp);
    ssd1306_bmp_show_image(&disp, SIDPOD_BMP, SIDPOD_BMP_SIZE);
    ssd1306_show(&disp);
}

void UI::showUI() {
    if (active) {
        if (visualize) {
            DanceFloor::start(PSIDCatalog::getCurrentEntry());
        } else {
            showSongSelector();
        }
    } else {
        showRasterBars();
    }
}

void UI::showSongSelector() {
    ssd1306_clear(&disp);
    PSIDCatalog::getWindow();
    uint8_t y = 0;
    for (auto entry: PSIDCatalog::getWindow()) {
        if (entry->selected) {
            ssd1306_draw_pixel(&disp, 0, y + 2);
            ssd1306_draw_line(&disp, 0, y + 3, 2, y + 3);
            ssd1306_draw_pixel(&disp, 0, y + 4);
        }
        ssd1306_draw_string(&disp, 4, y, 1, entry->title);
        y += 8;
    }
    ssd1306_show(&disp);
}

void UI::stop() {
    active = false;
    DanceFloor::stop();
}

void UI::start() {
    active = true;
    add_repeating_timer_ms(50, pollUserControls, nullptr, &userControlTimer);
    screenOn();
}

inline void UI::showRasterBars() {
    ssd1306_clear(&disp);
    int y = random() % (32);
    ssd1306_draw_line(&disp, 0, y, 127, y);
    unacked_ssd1306_show(&disp);
}

void UI::checkButtonPushed() {
    bool currentState = !gpio_get(ENC_SW);
    if (currentState && currentState != lastButtonState) {
        SIDPlayer::play();
        visualize = true;
    }
    lastButtonState = currentState;
}

void UI::pollForSongSelection() {
    encNewValue = quadrature_encoder_get_count(pio1, 1) / 2;
    encDelta = encNewValue - encOldValue;
    encOldValue = encNewValue;

    if (encDelta != 0) {
        if (encDelta > 0) {
            for (int i = 0; i < encDelta; i++) {
                PSIDCatalog::selectNext();
            }
        } else if (encDelta < 0) {
            for (int i = 0; i < encDelta * -1; i++) {
                PSIDCatalog::selectPrevious();
            }
        }
        visualize = false;
        DanceFloor::stop();
    }
}

bool UI::pollUserControls(struct repeating_timer *t) {
    checkButtonPushed();
    pollForSongSelection();
    return true;
}
