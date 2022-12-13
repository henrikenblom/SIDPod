#ifndef SIDPOD_SYSTEM_H
#define SIDPOD_SYSTEM_H

class System {
public:
    static void configureClocks();

    static void goDormant();

    static void softReset();

    static void enableUsb();

    static bool usbConnected();

    static void disableUsb();

private:
    static void sleepUntilDoubleClick();

    static bool repeatingTudTask(struct repeating_timer *t);
};

#endif //SIDPOD_SYSTEM_H
