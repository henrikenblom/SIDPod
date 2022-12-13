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
bool lastSwitchState = false;
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
char volumeLabel[7] = "Volume";
char lineLevelLabel[12] = "Line level:";
char yesLabel[4] = "Yes";
char noLabel[3] = "No";
float longTitleScrollOffset = 0;
float playingSymbolAnimationCounter = 0;
Visualization::DanceFloor *danceFloor;

void UI::initUI() {
    gpio_init(ENC_SW_PIN);
    gpio_set_dir(ENC_SW_PIN, GPIO_IN);
    gpio_pull_up(ENC_SW_PIN);
    uint offset = pio_add_program(pio1, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio1, ENC_SM, offset, ENC_BASE_PIN, 0);
    danceFloor = new Visualization::DanceFloor(&disp);
}

void UI::screenOn() {
    i2c_init(DISPLAY_I2C_BLOCK, I2C_BAUDRATE);
    gpio_set_function(DISPLAY_GPIO_BASE_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_GPIO_BASE_PIN + 1, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_GPIO_BASE_PIN);
    gpio_pull_up(DISPLAY_GPIO_BASE_PIN + 1);
    disp.external_vcc = DISPLAY_EXTERNAL_VCC;
    ssd1306_init(&disp, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_I2C_ADDRESS, i2c1);
    ssd1306_poweron(&disp);
    busy_wait_ms(DISPLAY_STATE_CHANGE_DELAY_MS);
    screenSleeping = false;
}

void UI::screenOff() {
    screenSleeping = true;
    ssd1306_clear(&disp);
    ssd1306_show(&disp);
    ssd1306_poweroff(&disp);
    i2c_deinit(DISPLAY_I2C_BLOCK);
}

void UI::showSplash() {
    ssd1306_clear(&disp);
    ssd1306_bmp_show_image(&disp, SIDPOD_BMP, SIDPOD_BMP_SIZE);
    ssd1306_show(&disp);
}

void UI::showUI() {
    if (active) {
        if (!screenSleeping) {
            if (visualize) {
                danceFloor->start(PSIDCatalog::getCurrentEntry());
            } else if (volumeControl) {
                showVolumeControl();
            } else {
                if (strcmp(PSIDCatalog::getCurrentEntry()->title, SIDPlayer::getCurrentlyLoaded()->title) == 0
                    && !SIDPlayer::loadingWasSuccessful()) {
                    PSIDCatalog::markCurrentEntryAsUnplayable();
                    SIDPlayer::resetState();
                    visualize = false;
                }
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
            if (entry->selected && strlen(entry->title) * 8 > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
                animateLongTitle(entry->title, y);
            } else {
                ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, y, 1, entry->title);
            }
            if (strcmp(entry->fileName, SIDPlayer::getCurrentlyLoaded()->fileName) == 0
                && SIDPlayer::loadingWasSuccessful()) {
                drawNowPlayingSymbol(y);
            } else if (entry->selected) {
                drawPlaySymbol(y);
            }
            if (entry->unplayable) crossOutLine(y);
            y += 8;
        }
        ssd1306_show(&disp);
    } else {
        showFlashEmptyScreen();
    }
}

void UI::animateLongTitle(char *title, int32_t y) {
    ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN - (int32_t) longTitleScrollOffset, y, 1, title);
    int scrollRange = (int) strlen(title) * FONT_WIDTH - DISPLAY_WIDTH + SONG_LIST_LEFT_MARGIN;
    float advancement =
            longTitleScrollOffset > 1 && (int) longTitleScrollOffset < scrollRange
            ? 0.4
            : 0.02;
    if ((int) (longTitleScrollOffset += advancement) > scrollRange)
        longTitleScrollOffset = 0;
    ssd1306_clear_square(&disp, 0, y, SONG_LIST_LEFT_MARGIN - 1, y + FONT_HEIGHT);
}

void UI::drawPlaySymbol(int32_t y) {
    ssd1306_draw_pixel(&disp, 0, y + 2);
    ssd1306_draw_line(&disp, 0, y + 3, 2, y + 3);
    ssd1306_draw_pixel(&disp, 0, y + 4);
}

void UI::drawNowPlayingSymbol(int32_t y) {
    int bar1 = 4;
    int bar2 = 1;
    int bar3 = 5;
    int bar4 = 3;
    if (SIDPlayer::isPlaying()) {
        bar1 = (int) playingSymbolAnimationCounter;
        bar2 = random() % (NOW_PLAYING_SYMBOL_HEIGHT);
        bar3 = (int) (sin(playingSymbolAnimationCounter) * (NOW_PLAYING_SYMBOL_HEIGHT - 2))
               + NOW_PLAYING_SYMBOL_HEIGHT - 3;
        bar4 = (int) ((NOW_PLAYING_SYMBOL_HEIGHT - bar1) * 0.5 + (bar2 * 0.5));
        if ((playingSymbolAnimationCounter += NOW_PLAYING_SYMBOL_ANIMATION_SPEED) >
            NOW_PLAYING_SYMBOL_HEIGHT)
            playingSymbolAnimationCounter = 0;
    }
    ssd1306_draw_line(&disp, 0, y + NOW_PLAYING_SYMBOL_HEIGHT - bar1, 0, y + NOW_PLAYING_SYMBOL_HEIGHT);
    ssd1306_draw_line(&disp, 1, y + NOW_PLAYING_SYMBOL_HEIGHT - bar2, 1, y + NOW_PLAYING_SYMBOL_HEIGHT);
    ssd1306_draw_line(&disp, 2, y + NOW_PLAYING_SYMBOL_HEIGHT - bar3, 2, y + NOW_PLAYING_SYMBOL_HEIGHT);
    ssd1306_draw_line(&disp, 3, y + NOW_PLAYING_SYMBOL_HEIGHT - bar4, 3, y + NOW_PLAYING_SYMBOL_HEIGHT);
}

void UI::crossOutLine(int32_t y) {
    ssd1306_draw_line(&disp, SONG_LIST_LEFT_MARGIN, y + 3, DISPLAY_WIDTH, y + 3);
}

void UI::showFlashEmptyScreen() {
    ssd1306_clear(&disp);
    char emptyFlashMsgLine1[17] = "No playable SIDs";
    char emptyFlashMsgLine2[16] = "found. Transfer";
    char emptyFlashMsgLine3[17] = "your funky PSIDs";
    char emptyFlashMsgLine4[11] = "using USB.";
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 0, 1, emptyFlashMsgLine1);
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 8, 1, emptyFlashMsgLine2);
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 16, 1, emptyFlashMsgLine3);
    ssd1306_draw_string(&disp, (DISPLAY_WIDTH / 2) - 64, 24, 1, emptyFlashMsgLine4);
    ssd1306_show(&disp);
}

void UI::showVolumeControl() {
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 4, 0, 1, volumeLabel);
    ssd13606_draw_empty_square(&disp, 4, 10, 120, 10);
    ssd1306_draw_square(&disp, 4, 10, 120 / VOLUME_STEPS * SIDPlayer::getVolume(), 10);
    ssd1306_draw_string(&disp, 4, 24, 1, lineLevelLabel);
    int statusLabelX = (int) (strlen(lineLevelLabel) * FONT_WIDTH) + FONT_WIDTH;
    if (SIDPlayer::lineLevelOn()) {
        ssd1306_draw_string(&disp, statusLabelX, 24, 1, yesLabel);
    } else {
        ssd1306_draw_string(&disp, statusLabelX, 24, 1, noLabel);
    }
    ssd1306_show(&disp);
}

void UI::stop() {
    active = false;
    danceFloor->stop();
}

void UI::start() {
    active = true;
    add_repeating_timer_ms(USER_CONTROLS_POLLRATE_MS, pollUserControls, nullptr, &userControlTimer);
}

inline void UI::showRasterBars() {
    ssd1306_clear(&disp);
    int y = random() % (DISPLAY_HEIGHT);
    ssd1306_draw_line(&disp, 0, y, DISPLAY_WIDTH - 1, y);
    ssd1306_show_unacked(&disp);
}

bool UI::pollUserControls(struct repeating_timer *t) {
    (void) t;
    pollSwitch();
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
                        longTitleScrollOffset = 0;
                    }
                }
            } else if (encDelta < 0) {
                for (int i = 0; i < encDelta * -1; i++) {
                    if (volumeControl) {
                        SIDPlayer::volumeDown();
                    } else {
                        PSIDCatalog::selectPrevious();
                        longTitleScrollOffset = 0;
                    }
                }
            }
            visualize = false;
            danceFloor->stop();
        }
    }
}

bool UI::pollSwitch() {
    bool used = false;
    bool currentSwitchState = !gpio_get(ENC_SW_PIN);
    if (currentSwitchState && currentSwitchState != lastSwitchState) {
        used = true;
        if (inDoubleClickSession) {
            endSingleClickSession();
            endDoubleClickSession();
            doubleClickCallback();
        } else {
            startDoubleClickSession();
            startSingleClickSession();
        }
        if (!screenSleeping) {
            if (inLongPressSession) {
                endLongPressSession();
            } else {
                startLongPressSession();
            }
        }
    } else if (!currentSwitchState && lastSwitchState) {
        endLongPressSession();
    }
    lastSwitchState = currentSwitchState;
    return used;
}

int64_t UI::singleClickCallback(alarm_id_t id, void *user_data) {
    (void) id;
    (void) user_data;
    endDoubleClickSession();
    if (!inLongPressSession && !screenSleeping) {
        if (visualize) {
            visualize = false;
            danceFloor->stop();
        } else if (volumeControl) {
            SIDPlayer::toggleLineLevel();
            resetVolumeControlSessionTimer();
        } else if (PSIDCatalog::getSize() && !PSIDCatalog::getCurrentEntry()->unplayable) {
            if (strcmp(SIDPlayer::getCurrentlyLoaded()->fileName,
                       PSIDCatalog::getCurrentEntry()->fileName) != 0) {
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
        goToSleep();
    }
    return 0;
}

void UI::doubleClickCallback() {
    if (screenSleeping) {
        screenOn();
    } else if (visualize || volumeControl) {
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
    (void) user_data;
    (void) id;
    endVolumeControlSession();
    visualize = true;
    return 0;
}

void UI::startVolumeControlSession() {
    visualize = false;
    volumeControl = true;
    danceFloor->stop();
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

void UI::goToSleep() {
    cancel_repeating_timer(&userControlTimer);
    System::disableUsb();
    danceFloor->stop();
    visualize = false;
    SIDPlayer::resetState();
    System::goDormant();
    screenOn();
    showSplash();
    System::enableUsb();
    lastSwitchState = false;
    start();
}
