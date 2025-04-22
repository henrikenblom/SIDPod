//
// Created by Henrik Enblom on 2025-04-21.
//

#ifndef BLUETOOTHDEVICELIST_H
#define BLUETOOTHDEVICELIST_H
#include <cstdint>
#include <cstring>
#include <vector>

#include "../platform_config.h"
#include "pico/types.h"

#if USE_BUDDY

#define BUDDY_ENABLE_PIN                    22

#define BUDDY_TAP_PIN                       21
#define BUDDY_VERTICAL_PIN                  20
#define BUDDY_HORIZONTAL_PIN                19
#define BUDDY_ROTATE_PIN                    18
#define BUDDY_MODIFIER1_PIN                 17
#define BUDDY_MODIFIER2_PIN                 16
#define BUDDY_BT_CONNECTION_PIN             15

#define MAX_CONNECTION_ATTEMPTS             3

enum RequestType {
    RT_NONE = 0,
    RT_BT_LIST = 1,
    RT_BT_SELECT = 2,
    RT_BT_DISCONNECT = 3,
};

struct Request {
    RequestType type;
    char data[32];
};

struct BluetoothDeviceListEntry {
    char name[32];
    bool selected;
};

class Buddy {
public:
    enum State {
        READY,
        REFRESHING,
        AWAITING_SELECTION,
        CONNECTING,
        CONNECTED,
        DISCONNECTED,
        AWAITING_STATE_CHANGE,
    };

    ~Buddy() {
        devices.clear();
        window.clear();
    }

    void addDevice(const char *name, bool selected = false);

    std::vector<BluetoothDeviceListEntry> *getWindow();

    void selectNext();

    void selectPrevious();

    void refreshDeviceList();

    [[nodiscard]] State getState() const {
        return state;
    }

    void connectSelected();

    void setDisconnected();

    void setConnecting();

    void setConnected() {
        state = CONNECTED;
    }

    static Buddy *getInstance();

protected:
    std::vector<BluetoothDeviceListEntry> devices;
    std::vector<BluetoothDeviceListEntry> window;
    uint8_t windowPosition = 0;
    uint8_t selectedPosition = 0;
    uint8_t windowSize = CATALOG_WINDOW_SIZE;
    uint8_t connectionAttempts = 0;
    State state = READY;

    Buddy();

    [[nodiscard]] size_t getSize() const;

    void updateWindow();

    void slideDown();

    void slideUp();

    void resetAccessors();

    bool awaitUartReadable();

    bool readBTDeviceName(char *buffer);

    void requestBTList();

    bool selectBTDevice(const char *deviceName);

    void disconnect();
};

#endif // USE_BUDDY

#endif //BLUETOOTHDEVICELIST_H
