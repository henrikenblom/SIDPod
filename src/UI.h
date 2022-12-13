#ifndef SIDPOD_UI_H
#define SIDPOD_UI_H

#include "display/include/ssd1306.h"

class UI {
public:
    static void initUI();

    static void showSplash();

    static void showUI();

    static void start();

    static void stop();

    static void screenOn();

    static void screenOff();

    static void goToSleep();

private:
    static void showSongSelector();

    static void drawPlaySymbol(int32_t y);

    static void drawNowPlayingSymbol(int32_t y);

    static void crossOutLine(int32_t y);

    static void animateLongTitle(char title[32], int32_t y);

    static void showFlashEmptyScreen();

    static void showVolumeControl();

    static inline void showRasterBars();

    static int64_t singleClickCallback(alarm_id_t id, void *user_data);

    static int64_t longPressCallback(alarm_id_t id, void *user_data);

    static int64_t endVolumeControlSessionCallback(alarm_id_t id, void *user_data);

    static void startVolumeControlSession();

    static void resetVolumeControlSessionTimer();

    static void endVolumeControlSession();

    static void doubleClickCallback();

    static void startSingleClickSession();

    static void startDoubleClickSession();

    static void startLongPressSession();

    static void endSingleClickSession();

    static void endDoubleClickSession();

    static void endLongPressSession();

    static bool pollSwitch();

    static void pollEncoder();

    static bool pollUserControls(struct repeating_timer *t);
};

#endif //SIDPOD_UI_H
