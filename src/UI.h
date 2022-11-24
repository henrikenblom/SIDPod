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

    static inline void showRasterBars();

    static void checkButtonPushed();

    static int64_t singleClickCallback(alarm_id_t id, void *user_data);

    static void pollForSongSelection();

    static bool pollUserControls(struct repeating_timer *t);
};

#endif //SIDPOD_UI_H
