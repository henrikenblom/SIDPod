#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <cstdio>
#include "UI.h"

#include <string>

#include "buddy/Buddy.h"
#include "Catalog.h"
#include "platform_config.h"
#include "display/include/ssd1306.h"
#include "Playlist.h"
#include "audio/SIDPlayer.h"
#include "quadrature_encoder.pio.h"
#include "visualization/DanceFloor.h"
#include "sidpod_24px_height_bmp.h"
#include "System.h"

ssd1306_t disp;
bool lastSwitchState, inDoubleClickSession, inLongPressSession, disconnectAffirmative = false;
int encNewValue, encDelta, encOldValue, showSplashCycles = 0;
repeating_timer userControlTimer;
alarm_id_t singleClickTimer, longPressTimer, showVolumeControlTimer;
auto volumeLabel = "VOLUME";
auto goingDormantLabel = "Shutting down...";
auto lineLevelLabel = "Line level:";
auto yesLabel = "Yes";
auto noLabel = "No";
float longTitleScrollOffset, headerScrollOffset, playingSymbolAnimationCounter = 0;
Visualization::DanceFloor *danceFloor;
Buddy *buddy = Buddy::getInstance();
UI::State currentState = UI::splash;
UI::State lastState = currentState;

void UI::initUI() {
#if (!USE_BUDDY)
    uint offset = pio_add_program(pio1, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio1, ENC_SM, offset, ENC_BASE_PIN, 0);
#endif
    gpio_init(SWITCH_PIN);
    gpio_set_dir(SWITCH_PIN, GPIO_IN);
    gpio_pull_up(SWITCH_PIN);
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
    currentState = sleeping;
    ssd1306_clear(&disp);
    ssd1306_show(&disp);
    ssd1306_poweroff(&disp);
    i2c_deinit(DISPLAY_I2C_BLOCK);
}

void UI::showSplash() {
    currentState = splash;
    ssd1306_clear(&disp);
    ssd1306_bmp_show_image(&disp, SIDPOD_24H_BMP, SIDPOD_24H_BMP_SIZE);
    ssd1306_draw_string(&disp, 0, 25, 1, "2.0");
    ssd1306_draw_string(&disp, 64, 25, 1, "\"Residious\"");
    ssd1306_show(&disp);
}

void UI::updateUI() {
    switch (currentState) {
        case visualization:
            if (Catalog::playlistIsOpen()) {
                Playlist *playlist = Catalog::getCurrentPlaylist();
                if (playlist->getState() == Playlist::State::READY) {
                    danceFloor->start(Catalog::getCurrentPlaylist()->getCurrentEntry());
                }
            }
            break;
        case volume_control:
            showVolumeControl();
            break;
        case raster_bars:
            showRasterBars();
            break;
        case splash:
            if (showSplashCycles == 0) {
                showSplash();
            }
            if (showSplashCycles++ > SPLASH_DISPLAY_DURATION) {
                showSplashCycles = 0;
#if USE_BUDDY
                currentState = bluetooth_interaction;
#else
                currentState = playlist_selector;
#endif
            } else {
                sleep_ms(1);
            }
            break;
        case playlist_selector:
            showPlaylistSelector();
            break;
        case bluetooth_interaction:
            showBluetoothInteraction();
            break;
        default:
            showSongSelector();
    }
    if (buddy->getState() != Buddy::CONNECTED) {
        currentState = bluetooth_interaction;
    }
}

void UI::drawHeader(const char *title) {
    const size_t stringWidth = strlen(title) * FONT_WIDTH;
    if (stringWidth > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
        animateLongTitle(title, 0, 12, &headerScrollOffset);
        ssd1306_draw_line(&disp, 0, 0, 8, 0);
        ssd1306_draw_line(&disp, 0, 2, 8, 2);
        ssd1306_draw_line(&disp, 0, 4, 8, 4);
        ssd1306_draw_line(&disp, 0, 6, 8, 6);
    } else {
        ssd1306_draw_line(&disp, 0, 0, DISPLAY_WIDTH, 0);
        ssd1306_draw_line(&disp, 0, 2, DISPLAY_WIDTH, 2);
        ssd1306_draw_line(&disp, 0, 4, DISPLAY_WIDTH, 4);
        ssd1306_draw_line(&disp, 0, 6, DISPLAY_WIDTH, 6);
        ssd1306_clear_square(&disp, 8, 0, stringWidth + FONT_WIDTH, FONT_HEIGHT);
        ssd1306_draw_string(&disp, 12, 0, 1, title);
    }
}

void UI::showSongSelector() {
    Playlist *playlist = Catalog::getCurrentPlaylist();
    auto playlistState = playlist->getState();
    if (playlistState == Playlist::State::READY) {
        currentState = song_selector;
        if (playlist->getSize()) {
            if (strcmp(playlist->getCurrentEntry()->title, SIDPlayer::getCurrentlyLoaded()->title) == 0
                && !SIDPlayer::loadingWasSuccessful()) {
                playlist->markCurrentEntryAsUnplayable();
                SIDPlayer::resetState();
            }
            ssd1306_clear(&disp);
            drawHeader(Catalog::getSelected().c_str());
            uint8_t y = 8;
            for (const auto entry: playlist->getWindow()) {
                if (entry->selected && strlen(entry->title) * FONT_WIDTH > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
                    animateLongTitle(entry->title, y, SONG_LIST_LEFT_MARGIN, &longTitleScrollOffset);
                } else {
                    ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, y, 1, entry->title);
                }
                if (Catalog::getPlaying() == playlist->getName() && strcmp(entry->fileName,
                                                                           SIDPlayer::getCurrentlyLoaded()->fileName) ==
                    0
                    && SIDPlayer::loadingWasSuccessful()) {
                    drawNowPlayingSymbol(y);
                } else if (entry->selected) {
                    drawOpenSymbol(y);
                }
                if (entry->unplayable) crossOutLine(y);
                y += 8;
            }
            ssd1306_show(&disp);
        }
    } else if (playlistState == Playlist::State::OUTDATED) {
        currentState = refreshing_playlist;
        ssd1306_clear(&disp);
        drawHeader(Catalog::getSelected().c_str());
        ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, 16, 1, "Loading...");
        ssd1306_show(&disp);
        playlist->refresh();
        busy_wait_ms(200);
    }
}

void UI::showPlaylistSelector() {
    ssd1306_clear(&disp);
    drawHeader("PLAYLISTS");
    uint8_t y = FONT_HEIGHT;
    for (const std::string &entry: *Catalog::getEntries()) {
        bool selected = entry == Catalog::getSelected();
        bool playing = entry == Catalog::getPlaying();
        if (selected && entry.length() * FONT_WIDTH > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
            animateLongTitle(entry.c_str(), y, SONG_LIST_LEFT_MARGIN, &longTitleScrollOffset);
        } else {
            ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, y, 1, entry.c_str());
        }
        if (selected) {
            drawOpenSymbol(y);
        } else if (playing && SIDPlayer::loadingWasSuccessful()) {
            drawNowPlayingSymbol(y);
        }
        y += 8;
    }
    ssd1306_show(&disp);
}


void UI::animateLongTitle(const char *title, int32_t y, int32_t xMargin, float *offsetCounter) {
    ssd1306_draw_string(&disp, xMargin - static_cast<int32_t>(*offsetCounter), y, 1, title);
    int scrollRange = (int) strlen(title) * FONT_WIDTH - DISPLAY_WIDTH + xMargin;
    float advancement =
            *offsetCounter > 1 && (int) *offsetCounter < scrollRange
                ? 0.4
                : 0.02;
    if (static_cast<int>(*offsetCounter += advancement) > scrollRange)
        *offsetCounter = 0;
    ssd1306_clear_square(&disp, 0, y, xMargin - 1, y + FONT_HEIGHT);
}

void UI::drawOpenSymbol(int32_t y) {
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

void UI::drawDialog(const char *text) {
    int labelWidth = static_cast<int>(strlen(text)) * FONT_WIDTH;
    int windowWidth = labelWidth + 4;
    int windowHeight = FONT_HEIGHT + 1;
    ssd1306_clear_square(&disp, DISPLAY_X_CENTER - (windowWidth / 2), DISPLAY_Y_CENTER - (windowHeight / 2) - 1,
                         windowWidth, windowHeight + 1);
    ssd13606_draw_empty_square(&disp, DISPLAY_X_CENTER - (windowWidth / 2), DISPLAY_Y_CENTER - (windowHeight / 2) - 1,
                               windowWidth, windowHeight + 1);
    ssd1306_draw_string(&disp, DISPLAY_X_CENTER - (labelWidth / 2) + 1, DISPLAY_Y_CENTER - FONT_HEIGHT / 2 + 1, 1,
                        text);
}

void UI::showVolumeControl() {
    ssd1306_clear(&disp);
    drawHeader(volumeLabel);
    ssd13606_draw_empty_square(&disp, 0, DISPLAY_HEIGHT / 2 - 4 + FONT_HEIGHT / 2,
                               DISPLAY_WIDTH - 1, 8);
    ssd1306_draw_square(&disp, 0, DISPLAY_HEIGHT / 2 - 4 + FONT_HEIGHT / 2,
                        static_cast<int>(
                            (DISPLAY_WIDTH - 1) * (static_cast<float>(SIDPlayer::getVolume()) / static_cast<float>(
                                                       VOLUME_STEPS))), 8);
    ssd1306_show(&disp);
}

void UI::stop() {
    currentState = raster_bars;
    danceFloor->stop();
}

void UI::start(bool quickStart) {
    if (quickStart) {
        showSplashCycles = SPLASH_DISPLAY_DURATION;
    }
    add_repeating_timer_ms(USER_CONTROLS_POLLRATE_MS, pollUserControls, nullptr, &userControlTimer);
}

inline void UI::showRasterBars() {
    ssd1306_clear(&disp);
    int y = random() % (DISPLAY_HEIGHT);
    ssd1306_draw_line(&disp, 0, y, DISPLAY_WIDTH - 1, y);
    ssd1306_show_unacked(&disp);
}

bool UI::pollUserControls(repeating_timer *t) {
    (void) t;
    pollSwitch();
#if (!USE_BUDDY)
    pollEncoder();
#endif
    return true;
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile bool UI::pollSwitch() {
    bool used = false;
    bool currentSwitchState = !gpio_get(SWITCH_PIN);
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
        if (currentState != sleeping) {
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

#if (!USE_BUDDY)

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void UI::pollEncoder() {
    encNewValue = quadrature_encoder_get_count(ENC_PIO, ENC_SM) / 2;
    encDelta = encNewValue - encOldValue;
    encOldValue = encNewValue;
    if (encDelta != 0) {
        verticalMovement(encDelta);
    }
}

#endif

#if USE_BUDDY

void UI::showBluetoothInteraction() {
    auto state = buddy->getState();
    switch (state) {
        case Buddy::READY:
            buddy->refreshDeviceList();
            break;
        case Buddy::REFRESHING:
            showBTProcessing("Scanning...");
            break;
        case Buddy::AWAITING_SELECTION:
            showBTDeviceList();
            break;
        case Buddy::CONNECTING:
        case Buddy::AWAITING_STATE_CHANGE:
            showBTProcessing("Connecting...");
            break;
        case Buddy::CONNECTED:
            currentState = playlist_selector;
            break;
        case Buddy::DISCONNECTED:
            showBTProcessing("Disconnected");
            buddy->refreshDeviceList();
            busy_wait_ms(2000);
            break;
        case Buddy::AWAITING_DISCONNECT_CONFIRMATION:
            showBTDisconnectConfirmation();
            break;
    }
}

void UI::showBTDisconnectConfirmation() {
    ssd1306_clear(&disp);
    drawHeader("BLUETOOTH");
    ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, FONT_HEIGHT, 1, "Disconnect?");
    ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, FONT_HEIGHT * 2, 1, yesLabel);
    ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, FONT_HEIGHT * 3, 1, noLabel);
    if (disconnectAffirmative) {
        drawOpenSymbol(FONT_HEIGHT * 2);
    } else {
        drawOpenSymbol(FONT_HEIGHT * 3);
    }
    ssd1306_show(&disp);
}

void UI::showBTProcessing(const char *text) {
    ssd1306_clear(&disp);
    drawHeader("BLUETOOTH");
    ssd1306_draw_string(&disp, 0, 16, 1, text);
    ssd1306_show(&disp);
}

void UI::showBTDeviceList() {
    ssd1306_clear(&disp);
    drawHeader("SELECT DEVICE");
    uint8_t y = FONT_HEIGHT;
    for (const auto &device: *buddy->getWindow()) {
        if (device.selected) {
            animateLongTitle(device.name, y, SONG_LIST_LEFT_MARGIN, &longTitleScrollOffset);
            drawOpenSymbol(y);
        } else {
            ssd1306_draw_string(&disp, SONG_LIST_LEFT_MARGIN, y, 1, device.name);
        }
        y += FONT_HEIGHT;
    }
    ssd1306_show(&disp);
}

void UI::adjustVolume(const bool up) {
    if (currentState == volume_control) {
        resetVolumeControlSessionTimer();
    } else {
        startVolumeControlSession();
        resetVolumeControlSessionTimer();
    }
    if (up) {
        SIDPlayer::volumeUp();
    } else {
        SIDPlayer::volumeDown();
    }
}

#endif

void UI::danceFloorStop() {
    danceFloor->stop();
}

void UI::verticalMovement(const int delta) {
    danceFloor->stop();
#if (!USE_BUDDY)
    if (currentState == visualization) {
        startVolumeControlSession();
        resetVolumeControlSessionTimer();
    } else if (currentState == volume_control) {
        resetVolumeControlSessionTimer();
    }
#endif

    if (delta > 0) {
        for (int i = 0; i < delta; i++) {
            if (currentState == volume_control) {
                SIDPlayer::volumeUp();
            } else if (currentState == bluetooth_interaction) {
                if (buddy->getState() == Buddy::AWAITING_DISCONNECT_CONFIRMATION) {
                    disconnectAffirmative = false;
                } else {
                    buddy->selectNext();
                }
            } else {
                if (Catalog::playlistIsOpen()) {
                    Catalog::getCurrentPlaylist()->selectNext();
                } else {
                    Catalog::selectNext();
                }
                longTitleScrollOffset = 0;
            }
        }
    } else if (delta < 0) {
        for (int i = 0; i < delta * -1; i++) {
            if (currentState == volume_control) {
                SIDPlayer::volumeDown();
            } else if (currentState == bluetooth_interaction) {
                if (buddy->getState() == Buddy::AWAITING_DISCONNECT_CONFIRMATION) {
                    disconnectAffirmative = true;
                } else {
                    buddy->selectPrevious();
                }
            } else {
                if (Catalog::playlistIsOpen()) {
                    Catalog::getCurrentPlaylist()->selectPrevious();
                } else {
                    Catalog::selectPrevious();
                }
                longTitleScrollOffset = 0;
            }
        }
    }
}

int64_t UI::singleClickCallback(alarm_id_t id, void *user_data) {
    (void) user_data;
    endDoubleClickSession();
    if (!inLongPressSession && currentState != sleeping) {
        if (id) {
            disconnectAffirmative = false;
            buddy->askToDisconnect();
            currentState = bluetooth_interaction;
        } else {
            switch (currentState) {
                case visualization:
                    currentState = song_selector;
                    danceFloor->stop();
                    break;
                case volume_control:
                    SIDPlayer::toggleLineLevel();
                    resetVolumeControlSessionTimer();
                    break;
                case playlist_selector:
                    Catalog::openSelected();
                    currentState = song_selector;
                    break;
                case refreshing_playlist:
                    break;
                case bluetooth_interaction:
                    if (buddy->getState() == Buddy::AWAITING_DISCONNECT_CONFIRMATION) {
                        if (disconnectAffirmative) {
                            buddy->disconnect();
                        } else {
                            buddy->setConnected();
                        }
                    } else {
                        buddy->connectSelected();
                    }
                    break;
                default:
                    auto playlist = Catalog::getCurrentPlaylist();
                    if (playlist->isAtReturnEntry()) {
                        Catalog::closeSelected();
                        currentState = playlist_selector;
                        break;
                    }
                    if (strcmp(SIDPlayer::getCurrentlyLoaded()->fileName,
                               playlist->getCurrentEntry()
                               ->fileName) != 0) {
                        SIDPlayer::togglePlayPause();
                    }
                    currentState = visualization;
            }
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
    while (!gpio_get(SWITCH_PIN)) {
        busy_wait_ms(1);
        if (i++ > DORMANT_ADDITIONAL_DURATION_MS) {
            goToSleep();
        }
    }
    return 0;
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void UI::doubleClickCallback() {
    if (currentState == sleeping) {
        screenOn();
    } else if (currentState == visualization
               || currentState == volume_control
               || currentState == song_selector) {
        SIDPlayer::togglePlayPause();
    }
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void UI::startSingleClickSession() {
    singleClickTimer = add_alarm_in_ms(DOUBLE_CLICK_SPEED_MS, singleClickCallback, nullptr, false);
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void UI::startDoubleClickSession() {
    inDoubleClickSession = true;
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void UI::startLongPressSession() {
    inLongPressSession = true;
    longPressTimer = add_alarm_in_ms(LONG_PRESS_DURATION_MS, longPressCallback, nullptr, false);
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void UI::endSingleClickSession() {
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
    currentState = lastState;
    return 0;
}

// ReSharper disable once CppDFAUnreachableFunctionCall
volatile void UI::startVolumeControlSession() {
    lastState = currentState;
    currentState = volume_control;
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

void UI::animateShutdown() {
    const int labelWidth = (int) strlen(goingDormantLabel) * FONT_WIDTH;
    constexpr int displayCenter = DISPLAY_WIDTH / 2;
    screenOn();
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, displayCenter - (labelWidth / 2) + 1, 13, 1, goingDormantLabel);
    ssd1306_show(&disp);
    busy_wait_ms(SPLASH_DISPLAY_DURATION);

    for (int i = 0; i < DISPLAY_HEIGHT / 2; i += 4) {
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, displayCenter - (labelWidth / 2) + 1, 13, 1, goingDormantLabel);
        ssd1306_clear_square(&disp, 0, 0, DISPLAY_WIDTH - 1, i);
        ssd1306_draw_line(&disp, 0, i, DISPLAY_WIDTH - 1, i);
        ssd1306_clear_square(&disp, 0, DISPLAY_HEIGHT, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - i);
        ssd1306_draw_line(&disp, 0, DISPLAY_HEIGHT - i, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - i);
        ssd1306_show(&disp);
        System::virtualVBLSync();
    }

    screenOff();
}

void UI::goToSleep() {
    cancel_repeating_timer(&userControlTimer);
    danceFloor->stop();
    SIDPlayer::resetState();
    animateShutdown();
    System::goDormant();
}
