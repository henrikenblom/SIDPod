#ifndef SIDPOD_UI_H
#define SIDPOD_UI_H

#include "display/include/ssd1306.h"

class UI {
public:
    static void initUI();

    static void showSplash();

    static void updateUI();

    static void drawHeader(const char *title);

    static void start(bool quickStart);

    static void stop();

    static void screenOn();

    static void screenOff();

    static void goToSleep();

    static void drawDialog(const char *text);

    enum State {
        song_selector, splash, raster_bars, visualization, volume_control, sleeping, playlist_selector
    };

private:
    static void showSongSelector();

    static void showPlaylistSelector();

    static void drawOpenSymbol(int32_t y);

    static void drawNowPlayingSymbol(int32_t y);

    static void crossOutLine(int32_t y);

    static void animateLongTitle(const char *title, int32_t y, int32_t xMargin, float *offsetCounter);

    static void drawRSIDSymbol(int32_t y);

    static void showVolumeControl();

    static inline void showRasterBars();

    static int64_t singleClickCallback(alarm_id_t id, void *user_data);

    static int64_t longPressCallback(alarm_id_t id, void *user_data);

    static int64_t endVolumeControlSessionCallback(alarm_id_t id, void *user_data);

    volatile static void startVolumeControlSession();

    static void resetVolumeControlSessionTimer();

    static void endVolumeControlSession();

    static void animateShutdown();

    volatile static void doubleClickCallback();

    volatile static void startSingleClickSession();

    volatile static void startDoubleClickSession();

    volatile static void startLongPressSession();

    volatile static void endSingleClickSession();

    static void endDoubleClickSession();

    static void endLongPressSession();

    volatile static bool pollSwitch();

    volatile static void pollEncoder();

    static bool pollUserControls(struct repeating_timer *t);
};

#endif //SIDPOD_UI_H
