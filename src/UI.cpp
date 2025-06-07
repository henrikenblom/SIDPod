#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <cstdio>
#include "UI.h"

#include <string>

#include "buddy/Buddy.h"
#include "Catalog.h"
#include "GL.h"
#include "platform_config.h"
#include "display/include/ssd1306.h"
#include "Playlist.h"
#include "audio/SIDPlayer.h"
#include "quadrature_encoder.pio.h"
#include "visualization/DanceFloor.h"
#include "sidpod_24px_height_bmp.h"
#include "System.h"

ssd1306_t disp;
GL gl(&disp);
bool lastSwitchState, inDoubleClickSession, inLongPressSession, disconnectAffirmative = false;
int encNewValue, encDelta, encOldValue, showSplashCycles = 0;
repeating_timer userControlTimer;
alarm_id_t singleClickTimer, longPressTimer, showVolumeControlTimer;
auto volumeLabel = "VOLUME";
auto goingDormantLabel = "Shutting down...";
auto yesLabel = "Yes";
auto noLabel = "No";
float longTitleScrollOffset, headerScrollOffset, playingSymbolAnimationCounter = 0;
auto danceFloor = new Visualization::DanceFloor(&gl);
UI::State currentState = UI::splash;
UI::State lastState = currentState;
#ifdef USE_BUDDY
auto *buddy = Buddy::getInstance();
#endif

void UI::initUI() {
#if (!USE_BUDDY)
    quadrature_encoder_program_init(pio1, ENC_SM, ENC_BASE_PIN, 0);
#endif
    gpio_init(SWITCH_PIN);
    gpio_set_dir(SWITCH_PIN, GPIO_IN);
    gpio_pull_up(SWITCH_PIN);
}

void UI::screenOn() {
    gl.displayOn();
    currentState = lastState;
}

void UI::screenOff() {
    lastState = currentState;
    currentState = sleeping;
    gl.displayOff();
}

void UI::showSplash() {
    currentState = splash;
    gl.clear();
    gl.showBMPImage(SIDPOD_24H_BMP, SIDPOD_24H_BMP_SIZE);
    gl.drawString(0, 25, "2.0beta");
    gl.drawString(64, 25, "\"Residious\"");
    gl.update();
}

void UI::initDanceFloor() {
    danceFloor->init();
}

void UI::startDanceFloor() {
    danceFloor->start();
}

void UI::updateUI() {
    switch (currentState) {
        case visualization:
            if (catalog->hasOpenPlaylist()) {
                if (const Playlist *playlist = catalog->getCurrentPlaylist();
                    playlist->getState() == Playlist::State::READY) {
#ifdef USE_BUDDY
                    buddy->forceRotationControl();
#endif
                    startDanceFloor();
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
#ifdef USE_BUDDY
            if (!catalog->findIsEnabled()) {
                buddy->forceVerticalControl();
            }
#endif
            showPlaylistSelector();
            break;
#ifdef USE_BUDDY
        case bluetooth_interaction:
            buddy->forceVerticalControl();
            showBluetoothInteraction();
            break;
#endif
        default:
#ifdef USE_BUDDY
            if (catalog->hasOpenPlaylist() && !catalog->getCurrentPlaylist()->findIsEnabled()) {
                buddy->forceVerticalControl();
            }
#endif
            showSongSelector();
    }
#ifdef USE_BUDDY
    if (buddy->getState() != Buddy::CONNECTED) {
        currentState = bluetooth_interaction;
    }
#endif
}

void UI::showSongSelector() {
    Playlist *playlist = catalog->getCurrentPlaylist();
    auto playlistState = playlist->getState();
    if (playlistState == Playlist::State::READY) {
        currentState = song_selector;
        if (playlist->getSize()) {
            if (strcmp(playlist->getCurrentEntry()->fileName, SIDPlayer::getCurrentlyLoaded()->fileName) == 0
                && !SIDPlayer::loadingWasSuccessful()) {
                playlist->markCurrentEntryAsUnplayable();
                SIDPlayer::resetState();
            }
            gl.clear();
            if (playlist->findIsEnabled()) {
                gl.drawInput("FIND:", playlist->getSearchTerm(), 8);
            } else {
                gl.drawHeader(playlist->getName());
            }
            uint8_t y = 8;
            for (const auto entry: playlist->getWindow()) {
                if (entry->selected && strlen(entry->getName()) * FONT_WIDTH > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
                    gl.animateLongText(entry->getName(), y, SONG_LIST_LEFT_MARGIN, &longTitleScrollOffset);
                } else {
                    gl.drawString(SONG_LIST_LEFT_MARGIN, y, entry->getName());
                }
                if (strcmp(entry->fileName, SIDPlayer::getCurrentlyLoaded()->fileName) == 0
                    && SIDPlayer::loadingWasSuccessful()) {
                    gl.drawNowPlayingSymbol(y);
                } else if (entry->selected) {
                    gl.drawOpenSymbol(y);
                }
                if (entry->unplayable) gl.crossoutLine(y);
                y += 8;
            }
            gl.update();
        }
    } else if (playlistState == Playlist::State::OUTDATED) {
        currentState = refreshing_playlist;
        gl.clear();
        gl.drawHeader(catalog->getCurrentPlaylist()->getName());
        gl.drawString(SONG_LIST_LEFT_MARGIN, 16, "Loading...");
        gl.update();
        playlist->refresh();
        busy_wait_ms(200);
    }
}

void UI::showPlaylistSelector() {
    gl.clear();
    if (catalog->findIsEnabled()) {
        gl.drawInput("FIND:", catalog->getSearchTerm(), 8);
    } else {
        gl.drawHeader("PLAYLISTS");
    }
    uint8_t y = FONT_HEIGHT;
    for (const auto &entry: catalog->getWindow()) {
        if (entry->selected && strlen(entry->name) * FONT_WIDTH > DISPLAY_WIDTH - SONG_LIST_LEFT_MARGIN) {
            gl.animateLongText(entry->name, y, SONG_LIST_LEFT_MARGIN, &longTitleScrollOffset);
        } else {
            gl.drawString(SONG_LIST_LEFT_MARGIN, y, entry->name);
        }
        if (entry->selected) {
            gl.drawOpenSymbol(y);
        } else if (entry->playing && SIDPlayer::loadingWasSuccessful()) {
            gl.drawNowPlayingSymbol(y);
        }
        y += 8;
    }
    gl.update();
}

void UI::showVolumeControl() {
    gl.clear();
    gl.drawHeader(volumeLabel);
    ssd13606_draw_empty_square(&disp, 0, DISPLAY_HEIGHT / 2 - 4 + FONT_HEIGHT / 2,
                               DISPLAY_WIDTH - 1, 8);
    ssd1306_draw_square(&disp, 0, DISPLAY_HEIGHT / 2 - 4 + FONT_HEIGHT / 2,
                        static_cast<int>(
                            (DISPLAY_WIDTH - 1) * (static_cast<float>(SIDPlayer::getVolume()) / static_cast<float>(
                                                       VOLUME_STEPS))), 8);
    gl.update();
}

void UI::stop() {
    currentState = raster_bars;
    danceFloor->stop();
}

void UI::start(bool quickStart) {
    if (quickStart) {
        showSplashCycles = SPLASH_DISPLAY_DURATION;
    }
#ifdef USE_BUDDY
    buddy->init();
#endif
    add_repeating_timer_ms(USER_CONTROLS_POLLRATE_MS, pollUserControls, nullptr, &userControlTimer);
}

inline void UI::showRasterBars() {
    gl.clear();
    const int y = random() % (DISPLAY_HEIGHT);
    gl.drawLine(0, y, DISPLAY_WIDTH - 1, y);
    gl.drawModal("USB CONNECTED");
    gl.update();
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
            showBTConnecting();
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

void UI::showBTConnecting() {
    gl.clear();
    gl.drawHeader("BLUETOOTH");
    auto selectedDeviceName = buddy->getSelectedDeviceName();
    char connectingMessage[64];
    snprintf(connectingMessage, sizeof(connectingMessage), "Connecting %.*s",
             static_cast<int>(strlen(selectedDeviceName) - 1), selectedDeviceName);
    gl.animateLongText(connectingMessage, FONT_HEIGHT, SONG_LIST_LEFT_MARGIN, &headerScrollOffset);
    gl.update();
}

void UI::showBTDisconnectConfirmation() {
    gl.clear();
    gl.drawHeader("BLUETOOTH");
    auto connectedDeviceName = buddy->getConnectedDeviceName();
    char disconnectMessage[64];
    snprintf(disconnectMessage, sizeof(disconnectMessage), "Disconnect %.*s?",
             static_cast<int>(strlen(connectedDeviceName) - 1), connectedDeviceName);
    gl.animateLongText(disconnectMessage, FONT_HEIGHT, SONG_LIST_LEFT_MARGIN, &headerScrollOffset);
    gl.drawString(SONG_LIST_LEFT_MARGIN, FONT_HEIGHT * 2, yesLabel);
    gl.drawString(SONG_LIST_LEFT_MARGIN, FONT_HEIGHT * 3, noLabel);
    if (disconnectAffirmative) {
        gl.drawOpenSymbol(FONT_HEIGHT * 2);
    } else {
        gl.drawOpenSymbol(FONT_HEIGHT * 3);
    }
    gl.update();
}

void UI::showBTProcessing(const char *text) {
    gl.clear();
    gl.drawHeader("BLUETOOTH");
    gl.drawString(0, 16, text);
    gl.update();
}

void UI::showBTDeviceList() {
    gl.clear();
    gl.drawHeader("SELECT BLUETOOTH DEVICE");
    uint8_t y = FONT_HEIGHT;
    for (const auto &device: *buddy->getWindow()) {
        if (device.selected) {
            gl.animateLongText(device.name, y, SONG_LIST_LEFT_MARGIN, &longTitleScrollOffset);
            gl.drawOpenSymbol(y);
        } else {
            gl.drawString(SONG_LIST_LEFT_MARGIN, y, device.name);
        }
        y += FONT_HEIGHT;
    }
    gl.update();
}

bool UI::allowFindFunctionality() {
    return currentState == song_selector
           || currentState == playlist_selector;
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

UI::State UI::getState() {
    return currentState;
}

void UI::screenshotToPBM() {
    ssd1306_dump_pbm(&disp);
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
#ifdef USE_BUDDY
            } else if (currentState == bluetooth_interaction) {
                if (buddy->getState() == Buddy::AWAITING_DISCONNECT_CONFIRMATION) {
                    disconnectAffirmative = false;
                } else {
                    buddy->selectNext();
                }
#endif
            } else {
                if (catalog->hasOpenPlaylist()) {
                    catalog->getCurrentPlaylist()->selectNext();
                } else {
                    catalog->selectNext();
                }
                longTitleScrollOffset = 0;
            }
        }
    } else if (delta < 0) {
        for (int i = 0; i < delta * -1; i++) {
            if (currentState == volume_control) {
                SIDPlayer::volumeDown();
#ifdef USE_BUDDY
            } else if (currentState == bluetooth_interaction) {
                if (buddy->getState() == Buddy::AWAITING_DISCONNECT_CONFIRMATION) {
                    disconnectAffirmative = true;
                } else {
                    buddy->selectPrevious();
                }
#endif
            } else {
                if (catalog->hasOpenPlaylist()) {
                    catalog->getCurrentPlaylist()->selectPrevious();
                } else {
                    catalog->selectPrevious();
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
#ifdef USE_BUDDY
        if (id) {
            disconnectAffirmative = false;
            buddy->askToDisconnect();
            lastState = currentState;
            currentState = bluetooth_interaction;
            danceFloor->stop();
        } else {
#endif

            switch (currentState) {
                case visualization:
                    currentState = song_selector;
                    danceFloor->stop();
                    break;
                case playlist_selector:
                    catalog->openSelected();
                    currentState = song_selector;
                    break;
#ifdef USE_BUDDY
                case refreshing_playlist:
                    break;
                case bluetooth_interaction:
                    if (buddy->getState() == Buddy::AWAITING_DISCONNECT_CONFIRMATION) {
                        if (disconnectAffirmative) {
                            buddy->disconnect();
                        } else {
                            buddy->setConnected();
                            currentState = lastState;
                        }
                    } else {
                        buddy->connectSelected();
                    }
                    break;
#endif
                default:
                    auto playlist = catalog->getCurrentPlaylist();
                    playlist->disableFind();
                    if (playlist->isAtReturnEntry()) {
                        catalog->closeSelected();
                        currentState = playlist_selector;
                        break;
                    }
                    if (strcmp(SIDPlayer::getCurrentlyLoaded()->fileName,
                               playlist->getCurrentEntry()
                               ->fileName) != 0) {
                        SIDPlayer::togglePlayPause();
                        initDanceFloor();
                    }
                    currentState = visualization;
            }
#ifdef USE_BUDDY
        }
#endif
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
    gl.clear();
    gl.drawString(displayCenter - (labelWidth / 2) + 1, 13, goingDormantLabel);
    gl.update();
    busy_wait_ms(SPLASH_DISPLAY_DURATION);

    for (int i = 0; i < DISPLAY_HEIGHT / 2; i += 4) {
        gl.clear();
        gl.drawString(displayCenter - (labelWidth / 2) + 1, 13, goingDormantLabel);
        ssd1306_clear_square(&disp, 0, 0, DISPLAY_WIDTH - 1, i);
        ssd1306_draw_line(&disp, 0, i, DISPLAY_WIDTH - 1, i);
        ssd1306_clear_square(&disp, 0, DISPLAY_HEIGHT, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - i);
        ssd1306_draw_line(&disp, 0, DISPLAY_HEIGHT - i, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - i);
        gl.update();
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
