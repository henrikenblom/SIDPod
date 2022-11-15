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

ssd1306_t disp;

void UI::initUI() {
    i2c_init(i2c1, I2C_BAUDRATE);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);
    disp.external_vcc = false;
    ssd1306_init(&disp, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_I2C_ADDRESS, i2c1);
}

void UI::showUI() {
    showSongSelector();
}

void UI::showSongSelector() {
    ssd1306_clear(&disp);
    PSIDCatalog::getWindow();
    uint8_t y = 2;
    for (auto entry: PSIDCatalog::getWindow()) {
        if (entry.selected) {
            ssd1306_draw_line(&disp, 0, y - 2, 127, y - 2);
            ssd1306_draw_line(&disp, 0, y + 8, 127, y + 8);
        }
        ssd1306_draw_string(&disp, 0, y, 1, entry.title);
        y += 10;
    }
    ssd1306_show(&disp);
}
