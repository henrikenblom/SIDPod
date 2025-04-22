//
// Created by Henrik Enblom on 2025-04-21.
//

#ifndef BLUETOOTHDEVICELIST_H
#define BLUETOOTHDEVICELIST_H
#include <cstdint>
#include <cstring>
#include <vector>

#include "platform_config.h"

struct BluetoothDeviceListEntry {
    char name[32];
    bool selected;
};

class BluetoothDeviceList {
public:
    enum State {
        OUTDATED,
        REFRESHING,
        READY,
        CONNECTING,
        CONNECTED,
    };

    BluetoothDeviceList() = default;

    ~BluetoothDeviceList() {
        devices.clear();
        window.clear();
    }

    void addDevice(const char *name);

    std::vector<BluetoothDeviceListEntry> *getWindow();

    void selectNext();

    void selectPrevious();

    void refresh();

    [[nodiscard]] State getState() const {
        return state;
    }

    void connectSelected();

    void awaitConnection();

    void setConnected() {
        state = CONNECTED;
    }

private:
    std::vector<BluetoothDeviceListEntry> devices;
    std::vector<BluetoothDeviceListEntry> window;
    uint8_t windowPosition = 0;
    uint8_t selectedPosition = 0;
    uint8_t windowSize = CATALOG_WINDOW_SIZE;
    State state = OUTDATED;

    [[nodiscard]] size_t getSize() const;

    void updateWindow();

    void slideDown();

    void slideUp();

    void resetAccessors();
};


#endif //BLUETOOTHDEVICELIST_H
