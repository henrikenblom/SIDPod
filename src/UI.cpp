#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <cstdio>
#include "UI.h"
#include "platform_config.h"
#include "display/include/ssd1306.h"
#include "PSIDCatalog.h"
#include "audio/SIDPlayer.h"
#include "quadrature_encoder.pio.h"
#include "visualization/DanceFloor.h"
#include "sidpod_bmp.h"
#include "System.h"

ssd1306_t disp;
bool active = false;
bool lastButtonState = false;
bool inDoubleClickSession = false;
bool inLongPressSession = false;
bool visualize = false;
bool volumeControl = false;
bool screenSleeping = false;
int encNewValue, encDelta, encOldValue = 0;
struct repeating_timer userControlTimer;
alarm_id_t singleClickTimer;
alarm_id_t longPressTimer;
alarm_id_t showVolumeControlTimer;
alarm_id_t clearScreenTimer;
char volumeLabel[7] = "Volume";

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
    ssd1306_poweron(&disp);
    busy_wait_ms(SCREEN_STATE_CHANGE_DELAY_MS);
    screenSleeping = false;
}

void UI::screenOff() {
    DanceFloor::stopWithCallback(powerOffScreenCallback);
    screenSleeping = true;
}

void UI::showSplash() {
    ssd1306_clear(&disp);
    ssd1306_bmp_show_image(&disp, SIDPOD_BMP, SIDPOD_BMP_SIZE);
    ssd1306_show(&disp);
}

void UI::showUI() {
    if (active) {
        if (!screenSleeping) {
            if (visualize && SIDPlayer::isPlaying()) {
                DanceFloor::start(PSIDCatalog::getCurrentEntry());
            } else if (volumeControl) {
                showVolumeControl();
            } else {
                showSongSelector();
            }
        }
    } else {
        showRasterBars();
    }
}

void UI::showSongSelector() {
    if (PSIDCatalog::getSize()) {
        ssd1306_clear(&disp);
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
    } else {
        showFlashEmptyScreen();
    }
}

void UI::showFlashEmptyScreen() {
    ssd1306_clear(&disp);
    char emptyFlashMsgLine1[17] = "No playable SIDs";
    char emptyFlashMsgLine2[16] = "found. Transfer";
    char emptyFlashMsgLine3[17] = "your funky tunes";
    char emptyFlashMsgLine4[11] = "using USB.";
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 0, 1, emptyFlashMsgLine1);
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 8, 1, emptyFlashMsgLine2);
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 16, 1, emptyFlashMsgLine3);
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 24, 1, emptyFlashMsgLine4);
    ssd1306_show(&disp);
}

void UI::showVolumeControl() {
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 4, 4, 1, volumeLabel);
    ssd13606_draw_empty_square(&disp, 4, 13, 120, 10);
    ssd1306_draw_square(&disp, 4, 13, 120 / VOLUME_STEPS * SIDPlayer::getVolume(), 10);
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
    int y = random() % (DISPLAY_HEIGHT);
    ssd1306_draw_line(&disp, 0, y, DISPLAY_WIDTH - 1, y);
    unacked_ssd1306_show(&disp);
}

bool UI::pollUserControls(struct repeating_timer *t) {
    (void) t;
    if (pollSwitch() && screenSleeping) {
        screenOn();
    }
    pollEncoder();
    return true;
}

void UI::pollEncoder() {
    encNewValue = quadrature_encoder_get_count(ENC_PIO, ENC_SM) / 2;
    encDelta = encNewValue - encOldValue;
    encOldValue = encNewValue;

    if (encDelta != 0) {
        if (!screenSleeping) {
            if (visualize) {
                startVolumeControlSession();
            }
            if (volumeControl) {
                resetVolumeControlSessionTimer();
            }
            if (encDelta > 0) {
                for (int i = 0; i < encDelta; i++) {
                    if (volumeControl) {
                        SIDPlayer::volumeUp();
                    } else {
                        PSIDCatalog::selectNext();
                    }
                }
            } else if (encDelta < 0) {
                for (int i = 0; i < encDelta * -1; i++) {
                    if (volumeControl) {
                        SIDPlayer::volumeDown();
                    } else {
                        PSIDCatalog::selectPrevious();
                    }
                }
            }
            visualize = false;
            DanceFloor::stop();
        }
    }
}

bool UI::pollSwitch() {
    bool used = false;
    bool currentState = !gpio_get(ENC_SW_PIN);
    if (currentState && currentState != lastButtonState) {
        used = true;
        if (!screenSleeping) {
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
        }
    } else if (!currentState && lastButtonState) {
        endLongPressSession();
    }
    lastButtonState = currentState;
    return used;
}

int64_t UI::singleClickCallback(alarm_id_t id, void *user_data) {
    (void) id;
    (void) user_data;
    endDoubleClickSession();
    if (!inLongPressSession) {
        if (visualize) {
            visualize = false;
            DanceFloor::stop();
        } else if (volumeControl) {
            endVolumeControlSession();
            visualize = true;
        } else if (PSIDCatalog::getSize()) {
            if (strcmp(SIDPlayer::getCurrentlyLoaded()->fileInfo.altname,
                       PSIDCatalog::getCurrentEntry()->fileInfo.altname) != 0) {
                SIDPlayer::togglePlayPause();
            }
            visualize = true;
        }
    }
    return 0;
}

int64_t UI::longPressCallback(alarm_id_t id, void *user_data) {
    (void) id;
    (void) user_data;
    endLongPressSession();
    endDoubleClickSession();
    screenOff();
    int i = 0;
    while (!gpio_get(ENC_SW_PIN)) {
        busy_wait_ms(1);
        i++;
    }
    if (i > DORMANT_ADDITIONAL_DURATION_MS) {
        System::goDormant();
    }
    return 0;
}

void UI::doubleClickCallback() {
    if (visualize || volumeControl) {
        SIDPlayer::togglePlayPause();
    }
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

int64_t UI::endVolumeControlSessionCallback(alarm_id_t id, void *user_data) {
    endVolumeControlSession();
    visualize = true;
    return 0;
}

void UI::startVolumeControlSession() {
    visualize = false;
    volumeControl = true;
    DanceFloor::stop();
}

void UI::resetVolumeControlSessionTimer() {
    if (showVolumeControlTimer) {
        cancel_alarm(showVolumeControlTimer);
    }
    showVolumeControlTimer = add_alarm_in_ms(VOLUME_CONTROL_DISPLAY_TIMEOUT,
                                             endVolumeControlSessionCallback,
                                             nullptr,
                                             false);
}

void UI::endVolumeControlSession() {
    if (showVolumeControlTimer) {
        cancel_alarm(showVolumeControlTimer);
    }
    volumeControl = false;
}

void UI::initDisplay() {
    ssd1306_init(&disp, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_I2C_ADDRESS, i2c1);
}

void UI::powerOffScreenCallback() {
    ssd1306_clear(&disp);
    ssd1306_show(&disp);
    ssd1306_poweroff(&disp);
}
