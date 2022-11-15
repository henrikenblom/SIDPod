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
unsigned long time = to_ms_since_boot(get_absolute_time());
const int delayTime = 50;

void UI::initUI() {
    i2c_init(i2c1, I2C_BAUDRATE);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);
    disp.external_vcc = false;
    ssd1306_init(&disp, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_I2C_ADDRESS, i2c1);

    gpio_init(ENC_SW);                    //Initialise a GPIO for (enabled I/O and set func to GPIO_FUNC_SIO)
    gpio_set_dir(ENC_SW, GPIO_IN);
    gpio_pull_up(ENC_SW);

    gpio_init(ENC_A);
    gpio_set_dir(ENC_A, GPIO_IN);
    gpio_disable_pulls(ENC_A);

    gpio_init(ENC_B);
    gpio_set_dir(ENC_B, GPIO_IN);
    gpio_disable_pulls(ENC_B);

    gpio_set_irq_enabled_with_callback(ENC_SW, GPIO_IRQ_EDGE_RISE, true, &encoderCallback);
    gpio_set_irq_enabled(ENC_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENC_B, GPIO_IRQ_EDGE_FALL, true);
}

void UI::showUI() {
    showSongSelector();
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
}

void UI::encoderCallback(uint gpio, __attribute__((unused)) uint32_t events) {
    uint32_t gpio_state = (gpio_get_all() >> 10) & 0b0111;

    static bool ccw_fall = false;
    static bool cw_fall = false;

    uint8_t enc_value = (gpio_state & 0x03);

    if (gpio == ENC_SW) {
        if ((to_ms_since_boot(get_absolute_time()) - time) > delayTime) {
            time = to_ms_since_boot(get_absolute_time());
            SIDPlayer::loadPSID(PSIDCatalog::getCurrentEntry());
            SIDPlayer::play();
        }
    }
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
