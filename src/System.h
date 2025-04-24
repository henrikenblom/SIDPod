#ifndef SIDPOD_SYSTEM_H
#define SIDPOD_SYSTEM_H

class System {
public:
    static void goDormant();

    static void softReset();

    static void hardReset();

    static void virtualVBLSync();

    static void enableUsb();

    static bool usbConnected();

private:
    static bool repeatingTudTask(struct repeating_timer *t);
};

#endif //SIDPOD_SYSTEM_H
