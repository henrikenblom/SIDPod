//
// Created by Henrik Enblom on 2025-04-21.
//

#include "Buddy.h"
#include "Catalog.h"
#include "UI.h"
#include "audio/SIDPlayer.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

Buddy *instance = nullptr;

Buddy *Buddy::getInstance() {
    if (instance == nullptr) {
        instance = new Buddy();
    }
    return instance;
}

void buddyCallback(uint gpio, uint32_t events) {
    bool mod1 = gpio_get(BUDDY_MODIFIER1_PIN);
    bool mod2 = gpio_get(BUDDY_MODIFIER2_PIN);
    switch (gpio) {
        case BUDDY_TAP_PIN:
            if (mod1) {
                if (mod2) {
                    if (Catalog::playlistIsOpen()) {
                        Catalog::getCurrentPlaylist()->resetAccessors();
                    } else {
                        Catalog::goHome();
                    }
                } else {
                    UI::doubleClickCallback();
                }
            } else {
                UI::singleClickCallback(0, nullptr);
            }
            break;
        case BUDDY_VERTICAL_PIN:
            UI::verticalMovement(mod1 ? -1 : 1);
            break;
        case BUDDY_HORIZONTAL_PIN:
            break;
        case BUDDY_ROTATE_PIN:
            UI::adjustVolume(mod1);
            break;
        case BUDDY_BT_CONNECTION_PIN:
            auto buddy = Buddy::getInstance();
            if (mod1 && mod2) {
                buddy->setConnecting();
            } else if (mod1) {
                buddy->setDisconnected();
            } else if (mod2) {
                if (buddy->getState() != Buddy::REFRESHING) {
                    buddy->refreshDeviceList();
                }
            } else {
                buddy->setConnected();
            }
            break;
    }
}

Buddy::Buddy() {
    gpio_init(BUDDY_ENABLE_PIN);
    gpio_set_dir(BUDDY_ENABLE_PIN, GPIO_OUT);
    gpio_set_drive_strength(BUDDY_ENABLE_PIN, GPIO_DRIVE_STRENGTH_12MA);
    gpio_put(BUDDY_ENABLE_PIN, true);

    gpio_init(BUDDY_TAP_PIN);
    gpio_init(BUDDY_VERTICAL_PIN);
    gpio_init(BUDDY_HORIZONTAL_PIN);
    gpio_init(BUDDY_ROTATE_PIN);
    gpio_init(BUDDY_MODIFIER1_PIN);
    gpio_init(BUDDY_MODIFIER2_PIN);
    gpio_init(BUDDY_BT_CONNECTION_PIN);

    gpio_set_dir(BUDDY_MODIFIER1_PIN, GPIO_IN);
    gpio_set_dir(BUDDY_MODIFIER2_PIN, GPIO_IN);
    gpio_set_dir(BUDDY_BT_CONNECTION_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(BUDDY_TAP_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
    gpio_set_irq_enabled_with_callback(BUDDY_VERTICAL_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
    gpio_set_irq_enabled_with_callback(BUDDY_HORIZONTAL_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
    gpio_set_irq_enabled_with_callback(BUDDY_ROTATE_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
    gpio_set_irq_enabled_with_callback(BUDDY_BT_CONNECTION_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);

    if (gpio_get(BUDDY_BT_CONNECTION_PIN)) {
        bool mod1 = gpio_get(BUDDY_MODIFIER1_PIN);
        bool mod2 = gpio_get(BUDDY_MODIFIER2_PIN);
        if (mod1 && mod2) {
            state = CONNECTING;
        } else if (mod1) {
            state = DISCONNECTED;
        } else if (!mod2) {
            state = CONNECTED;
        }
    }
}

void Buddy::addDevice(const char *name, const bool selected) {
    for (const auto &device: devices) {
        if (std::strcmp(device.name, name) == 0) {
            return;
        }
    }
    BluetoothDeviceListEntry newDevice{};
    std::strncpy(newDevice.name, name, sizeof(newDevice.name) - 1);
    newDevice.name[sizeof(newDevice.name) - 1] = '\0';
    newDevice.selected = selected;
    devices.push_back(newDevice);
}

std::vector<BluetoothDeviceListEntry> *Buddy::getWindow() {
    return &window;
}

void Buddy::selectNext() {
    if (state == AWAITING_SELECTION) {
        if (selectedPosition < getSize() - 1) {
            selectedPosition++;
            slideDown();
            updateWindow();
        }
    }
}

void Buddy::selectPrevious() {
    if (state == AWAITING_SELECTION) {
        if (selectedPosition > 0) {
            selectedPosition--;
            slideUp();
            updateWindow();
        }
    }
}

void Buddy::refreshDeviceList() {
    state = REFRESHING;
    char *selectedDevice = nullptr;
    if (!devices.empty()) {
        selectedDevice = devices.at(selectedPosition).name;
        devices.clear();
    }
    requestBTList();
    char buffer[32] = {};
    while (readBTDeviceName(buffer)) {
        if (selectedDevice && std::strcmp(buffer, selectedDevice) == 0) {
            addDevice(buffer, true);
            selectedPosition = devices.size() - 1;
        } else {
            addDevice(buffer);
        }
        addDevice(buffer);
    }
    resetAccessors();
    state = AWAITING_SELECTION;
}

void Buddy::connectSelected() {
    selectBTDevice(devices.at(selectedPosition).name);
}

void Buddy::setDisconnected() {
    if (state == CONNECTED) {
        UI::danceFloorStop();
        SIDPlayer::pauseIfPlaying();
        state = DISCONNECTED;
    }
}

void Buddy::setConnecting() {
    if (connectionAttempts++ >= MAX_CONNECTION_ATTEMPTS) {
        disconnect();
        connectionAttempts = 0;
    } else {
        state = CONNECTING;
    }
}

size_t Buddy::getSize() const {
    return devices.size();
}

void Buddy::updateWindow() {
    if (getSize()) {
        window.clear();
        for (int i = 0; i < std::min(windowSize, static_cast<uint8_t>(getSize())); i++) {
            const auto entry = &devices.at(windowPosition + i);
            entry->selected = windowPosition + i == selectedPosition;
            window.push_back(*entry);
        }
    }
}

void Buddy::slideDown() {
    if (windowSize + windowPosition < getSize()) {
        windowPosition++;
    }
}

void Buddy::slideUp() {
    if (windowPosition > 0) {
        windowPosition--;
    }
}

void Buddy::resetAccessors() {
    selectedPosition = 0;
    windowPosition = 0;
    if (getSize() > 0) {
        updateWindow();
    }
}

bool Buddy::awaitUartReadable() {
    int c = 0;
    while (!uart_is_readable(UART_ID)) {
        busy_wait_ms(1);
        c++;
        if (c > 1000) {
            return false;
        }
    }
    return true;
}

bool Buddy::readBTDeviceName(char *buffer) {
    if (awaitUartReadable()) {
        for (int i = 0; i < 32; i++) {
            if (!awaitUartReadable()) {
                return false;
            }
            const uint8_t ch = uart_getc(UART_ID);
            buffer[i] = ch;
            if (ch == '\n') {
                break;
            }
        }
        return true;
    }
    return false;
}

void Buddy::requestBTList() {
    uart_putc(UART_ID, RT_BT_LIST);
    uart_putc(UART_ID, 0);
}

bool Buddy::selectBTDevice(const char *deviceName) {
    uart_putc(UART_ID, RT_BT_SELECT);
    uart_puts(UART_ID, deviceName);
    state = AWAITING_STATE_CHANGE;
    return true;
}

void Buddy::disconnect() {
    uart_putc(UART_ID, RT_BT_DISCONNECT);
    uart_putc(UART_ID, 0);
    state = READY;
}
