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
bool inDoubleClickSession = false;
bool inLongPressSession = false;
bool visualize = false;
int encNewValue, encDelta, encOldValue = 0;
struct repeating_timer userControlTimer;
alarm_id_t singleClickTimer;
alarm_id_t longPressTimer;

void UI::initUI() {
    i2c_init(DISP_I2C_BLOCK, I2C_BAUDRATE);
    gpio_set_function(DISP_GPIO_BASE_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISP_GPIO_BASE_PIN + 1, GPIO_FUNC_I2C);
    gpio_pull_up(DISP_GPIO_BASE_PIN);
    gpio_pull_up(DISP_GPIO_BASE_PIN + 1);
    gpio_init(ENC_SW_PIN);
    gpio_set_dir(ENC_SW_PIN, GPIO_IN);
    gpio_pull_up(ENC_SW_PIN);
    uint offset = pio_add_program(pio1, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio1, ENC_SM, offset, ENC_BASE_PIN, 0);
    disp.external_vcc = DISP_EXTERNAL_VCC;
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
    add_repeating_timer_ms(USER_CONTROLS_POLLRATE_MS, pollUserControls, nullptr, &userControlTimer);
    screenOn();
}

inline void UI::showRasterBars() {
    ssd1306_clear(&disp);
    int y = random() % (32);
    ssd1306_draw_line(&disp, 0, y, 127, y);
    unacked_ssd1306_show(&disp);
}

bool UI::pollUserControls(struct repeating_timer *t) {
    checkButtonPushed();
    pollForSongSelection();
    return true;
}

void UI::pollForSongSelection() {
    encNewValue = quadrature_encoder_get_count(ENC_PIO, ENC_SM) / 2;
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

void UI::checkButtonPushed() {
    bool currentState = !gpio_get(ENC_SW_PIN);
    if (currentState && currentState != lastButtonState) {
        if (inDoubleClickSession) {
            endSingleClickSession();
            endDoubleClickSession();
            doubleClickCallback();
        } else {
            startDoubleClickSession();
            startSingleClickSession();
        }
        if (inLongPressSession) {
            endLongPressSession();
        } else {
            startLongPressSession();
        }
    } else if (!currentState && lastButtonState) {
        endLongPressSession();
    }
    lastButtonState = currentState;
}

int64_t UI::singleClickCallback(alarm_id_t id, void *user_data) {
    (void) id;
    (void) user_data;
    endDoubleClickSession();
    if (!inLongPressSession) {
        printf("singleClickCallback\n");
        SIDPlayer::play();
        visualize = true;
    }
    return 0;
}

int64_t UI::longPressCallback(alarm_id_t id, void *user_data) {
    (void) id;
    (void) user_data;
    printf("longPressCallback\n");
    endLongPressSession();
    endDoubleClickSession();
    return 0;
}

void UI::doubleClickCallback() {
    printf("doubleClickCallBack\n");
}

void UI::startSingleClickSession() {
    singleClickTimer = add_alarm_in_ms(DOUBLE_CLICK_SPEED_MS, singleClickCallback, nullptr, false);
}

void UI::startDoubleClickSession() {
    inDoubleClickSession = true;
}

void UI::startLongPressSession() {
    inLongPressSession = true;
    longPressTimer = add_alarm_in_ms(LONG_PRESS_DURATION_MS, longPressCallback, nullptr, false);
}

void UI::endSingleClickSession() {
    if (singleClickTimer) cancel_alarm(singleClickTimer);
}

void UI::endDoubleClickSession() {
    inDoubleClickSession = false;
}

void UI::endLongPressSession() {
    if (longPressTimer) cancel_alarm(longPressTimer);
    inLongPressSession = false;
}
