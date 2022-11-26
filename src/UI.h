#ifndef SIDPOD_UI_H
#define SIDPOD_UI_H

#include "ssd1306.h"

class UI {
public:
    static void initUI();

    static void showSplash();

    static void showUI();

    static void start();

    static void stop();

    static void screenOn();

private:
    static void showSongSelector();

    static void showVolumeControl();

    static inline void showRasterBars();

    static void pollSwitch();

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

    static void pollEncoder();

    static bool pollUserControls(struct repeating_timer *t);
};

#endif //SIDPOD_UI_H
