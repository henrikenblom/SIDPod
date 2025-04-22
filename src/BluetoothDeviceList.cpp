//
// Created by Henrik Enblom on 2025-04-21.
//

#include "BluetoothDeviceList.h"

#include "System.h"

void BluetoothDeviceList::addDevice(const char *name) {
    for (const auto &device: devices) {
        if (std::strcmp(device.name, name) == 0) {
            return;
        }
    }
    BluetoothDeviceListEntry newDevice{};
    std::strncpy(newDevice.name, name, sizeof(newDevice.name) - 1);
    newDevice.name[sizeof(newDevice.name) - 1] = '\0';
    newDevice.selected = false;
    devices.push_back(newDevice);
}

std::vector<BluetoothDeviceListEntry> *BluetoothDeviceList::getWindow() {
    return &window;
}

void BluetoothDeviceList::selectNext() {
    if (state == READY) {
        if (selectedPosition < getSize() - 1) {
            selectedPosition++;
            slideDown();
            updateWindow();
        }
    }
}

void BluetoothDeviceList::selectPrevious() {
    if (state == READY) {
        if (selectedPosition > 0) {
            selectedPosition--;
            slideUp();
            updateWindow();
        }
    }
}

void BluetoothDeviceList::refresh() {
    state = REFRESHING;
    System::requestBTList();
    char buffer[32];
    while (System::readBTDeviceName(buffer)) {
        addDevice(buffer);
    }
    resetAccessors();
    state = READY;
}

void BluetoothDeviceList::connectSelected() {
    System::selectBTDevice(devices.at(selectedPosition).name);
    state = CONNECTING;
}

void BluetoothDeviceList::awaitConnection() {
    state = CONNECTING;
}

size_t BluetoothDeviceList::getSize() const {
    return devices.size();
}

void BluetoothDeviceList::updateWindow() {
    if (getSize()) {
        window.clear();
        for (int i = 0; i < std::min(windowSize, static_cast<uint8_t>(getSize())); i++) {
            const auto entry = &devices.at(windowPosition + i);
            entry->selected = windowPosition + i == selectedPosition;
            window.push_back(*entry);
        }
    }
}

void BluetoothDeviceList::slideDown() {
    if (windowSize + windowPosition < getSize()) {
        windowPosition++;
    }
}

void BluetoothDeviceList::slideUp() {
    if (windowPosition > 0) {
        windowPosition--;
    }
}

void BluetoothDeviceList::resetAccessors() {
    selectedPosition = 0;
    windowPosition = 0;
    if (getSize() > 0) {
        updateWindow();
    }
}
