#ifndef SIDPOD_UI_H
#define SIDPOD_UI_H

#define ENC_A    10
#define ENC_B    11

class UI {
public:
    static void initUI();

    static void showUI();

    static void start();

    static void stop();

private:
    static void showSongSelector();

    static void encoderCallback(uint gpio, __attribute__((unused)) uint32_t events);

    static inline void showRasterBars();
};

#endif //SIDPOD_UI_H
