#ifndef SIDPOD_SYSTEM_H
#define SIDPOD_SYSTEM_H

class System {
public:
    static void configureClocks();

    static void goDormant();

    static void softReset();

    static void hardReset();

    static void enableUsb();

    static void runExclusiveUsbDBurst();

    static bool usbConnected();

private:
    static bool repeatingTudTask(struct repeating_timer *t);
};

#endif //SIDPOD_SYSTEM_H
