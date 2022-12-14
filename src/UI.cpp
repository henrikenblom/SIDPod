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
bool lastSwitchState, inDoubleClickSession, inLongPressSession = false;
int encNewValue, encDelta, encOldValue = 0;
struct repeating_timer userControlTimer;
alarm_id_t singleClickTimer, longPressTimer, showVolumeControlTimer;
char volumeLabel[7] = "Volume";
char lineLevelLabel[12] = "Line level:";
char yesLabel[4] = "Yes";
char noLabel[3] = "No";
float longTitleScrollOffset, playingSymbolAnimationCounter = 0;
Visualization::DanceFloor *danceFloor;
UI::State currentState = UI::song_selector;
UI::State lastState = UI::song_selector;

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
    currentState = lastState;
}

void UI::screenOff() {
    lastState = currentState;
    currentState = UI::sleeping;
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

void UI::updateUI() {
    switch (currentState) {
        case UI::visualization:
            danceFloor->start(PSIDCatalog::getCurrentEntry());
            break;
        case UI::volume_control:
            showVolumeControl();
            break;
        case UI::raster_bars:
            showRasterBars();
            break;
        case UI::splash:
            showSplash();
            break;
        default:
            showSongSelector();
    }
}

void UI::showSongSelector() {
    if (PSIDCatalog::getSize()) {
        if (strcmp(PSIDCatalog::getCurrentEntry()->title, SIDPlayer::getCurrentlyLoaded()->title) == 0
            && !SIDPlayer::loadingWasSuccessful()) {
            PSIDCatalog::markCurrentEntryAsUnplayable();
            SIDPlayer::resetState();
        }
        ssd1306_clear(&disp);
        uint8_t y = 0;
        for (auto entry: PSIDCatalog::getWindow()) {
            if (entry->selected && strlen(entry->title) * FONT_WIDTH > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
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
    char emptyFlashMsgLine1[19] = "No playable PSIDs.";
    char emptyFlashMsgLine2[20] = "Use USB to transfer";
    char emptyFlashMsgLine3[18] = "PSIDs to the root";
    char emptyFlashMsgLine4[15] = "of the SIDPod.";
    int x = (DISPLAY_WIDTH / 2) - FONT_WIDTH * 10;
    ssd1306_draw_string(&disp, x, 0, 1, emptyFlashMsgLine1);
    ssd1306_draw_string(&disp, x, 8, 1, emptyFlashMsgLine2);
    ssd1306_draw_string(&disp, x, 16, 1, emptyFlashMsgLine3);
    ssd1306_draw_string(&disp, x, 24, 1, emptyFlashMsgLine4);
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
    currentState = UI::raster_bars;
    danceFloor->stop();
}

void UI::start() {
    currentState = UI::song_selector;
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
        danceFloor->stop();
        if (currentState == UI::visualization) {
            startVolumeControlSession();
        } else if (currentState == UI::volume_control) {
            resetVolumeControlSessionTimer();
        }
        if (encDelta > 0) {
            for (int i = 0; i < encDelta; i++) {
                if (currentState == UI::volume_control) {
                    SIDPlayer::volumeUp();
                } else {
                    PSIDCatalog::selectNext();
                    longTitleScrollOffset = 0;
                }
            }
        } else if (encDelta < 0) {
            for (int i = 0; i < encDelta * -1; i++) {
                if (currentState == UI::volume_control) {
                    SIDPlayer::volumeDown();
                } else {
                    PSIDCatalog::selectPrevious();
                    longTitleScrollOffset = 0;
                }
            }
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
        if (currentState != UI::sleeping) {
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
    if (!inLongPressSession && currentState != UI::sleeping) {
        switch (currentState) {
            case UI::visualization:
                currentState = UI::song_selector;
                danceFloor->stop();
                break;
            case UI::volume_control:
                SIDPlayer::toggleLineLevel();
                resetVolumeControlSessionTimer();
                break;
            default:
                if (strcmp(SIDPlayer::getCurrentlyLoaded()->fileName,
                           PSIDCatalog::getCurrentEntry()->fileName) != 0) {
                    SIDPlayer::togglePlayPause();
                }
                currentState = UI::visualization;
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
    if (currentState == UI::sleeping) {
        screenOn();
    } else if (currentState == UI::visualization || UI::volume_control) {
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
    currentState = UI::visualization;
    return 0;
}

void UI::startVolumeControlSession() {
    currentState = UI::volume_control;
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
}

void UI::goToSleep() {
    cancel_repeating_timer(&userControlTimer);
    System::disableUsb();
    danceFloor->stop();
    currentState = UI::splash;
    SIDPlayer::resetState();
    System::goDormant();
    screenOn();
    showSplash();
    System::enableUsb();
    lastSwitchState = false;
    start();
}
