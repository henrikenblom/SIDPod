#ifndef SIDPOD_SYSTEM_H
#define SIDPOD_SYSTEM_H

class System {
public:
    static void goDormant();

    static void softReset();

    static void hardReset();

    static void virtualVBLSync();

    static void enableUsb();

    static void runExclusiveUsbDBurst();

    static bool usbConnected();

    static bool awaitUartReadable();

    static bool readBTDeviceName(char *buffer);

    static void requestBTList();

    static void updateBTDeviceList();

    static bool selectBTDevice(const char *deviceName);

    static void on_uart_rx();

    static void initBuddy();

private:
    static bool repeatingTudTask(struct repeating_timer *t);
};

#endif //SIDPOD_SYSTEM_H
