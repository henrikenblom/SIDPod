#ifndef SIDPOD_UI_H
#define SIDPOD_UI_H

#define ENC_A    10
#define ENC_B    11
#define ENC_SW    12

class UI {
public:
    static void initUI();

    static void showUI();

    static void start();

    static void stop();

    static void screenOff();

    static void screenOn();

private:
    static void showSongSelector();

    static inline void showRasterBars();

    static void checkButtonPushed();

    static void pollForSongSelection();
};

#endif //SIDPOD_UI_H
