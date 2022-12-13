#ifndef SIDPOD_SYSTEM_H
#define SIDPOD_SYSTEM_H

class System {
public:
    static void configureClock();

    static void goDormant();

    static void softReset();

private:
    static void sleepUntilDoubleClick();
};

#endif //SIDPOD_SYSTEM_H
