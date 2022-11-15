#ifndef SIDPOD_UI_H
#define SIDPOD_UI_H

#define ENC_A    10
#define ENC_B    11
#define ENC_SW    12

class UI {
public:
    static void initUI();

    static void showUI();

private:
    static void showSongSelector();

    static void encoderCallback(uint gpio, __attribute__((unused)) uint32_t events);
};

#endif //SIDPOD_UI_H
