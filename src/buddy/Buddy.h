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
#define BUDDY_BT_CONNECTED_PIN              15
#define UART_READABLE_TIMEOUT_MS            100
#define MAX_CONNECTION_ATTEMPTS             3

enum RequestType {
    RT_NONE = 0,
    RT_BT_LIST = 1,
    RT_BT_SELECT = 2,
    RT_BT_DISCONNECT = 3,
    RT_G_FORCE_ROTATE = 4,
    RT_G_FORCE_VERTICAL = 5,
    RT_G_FORCE_HORIZONTAL = 6,
    RT_G_SET_AUTO = 7,
    RT_BT_GET_CONNECTED = 8,
};

enum NotificationType {
    NT_NONE = 0,
    NT_GESTURE = 1,
    NT_BT_CONNECTED = 2,
    NT_BT_DISCONNECTED = 3,
    NT_BT_DEVICE_LIST_CHANGED = 4,
    NT_BT_CONNECTING = 5,
};

enum Gesture {
    G_NONE = 0,
    G_HORIZONTAL = 1,
    G_VERTICAL = 2,
    G_ROTATE = 3,
    G_TAP = 4,
    G_DOUBLE_TAP = 5,
    G_HOME = 6,
};

struct Request {
    RequestType type;
    char data[32];
};

struct BluetoothDeviceListEntry {
    char name[32];
    bool selected;
    bool connected;
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
        AWAITING_DISCONNECT_CONFIRMATION,
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

    void setConnected();

    void askToDisconnect();

    void forceRotationControl();

    void forceVerticalControl();

    void enableGestureDetection();

    void disconnect();

    const char *getConnectedDeviceName() {
        return lastConnectedDeviceName;
    }

    const char *getSelectedDeviceName() {
        return selectedDeviceName;
    }

    static Buddy *getInstance();

protected:
    std::vector<BluetoothDeviceListEntry> devices;
    std::vector<BluetoothDeviceListEntry> window;
    uint8_t windowPosition = 0;
    uint8_t selectedPosition = 0;
    uint8_t windowSize = CATALOG_WINDOW_SIZE;
    uint8_t connectionAttempts = 0;
    RequestType lastGestureRequest = RT_NONE;
    const char *selectedDeviceName = nullptr;
    char lastConnectedDeviceName[32];
    uint8_t lastConnectedAddr[6];
    State state = READY;

    Buddy();

    [[nodiscard]] size_t getSize() const;

    void updateWindow();

    void slideDown();

    void slideUp();

    void resetAccessors();

    bool awaitUartReadable();

    bool readBTDeviceName(char *buffer);

    void requestAndSetConnectedBTDeviceName();

    void requestBTList();

    bool selectBTDevice(const char *deviceName);
};

#endif // USE_BUDDY

#endif //BLUETOOTHDEVICELIST_H
