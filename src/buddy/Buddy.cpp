//
// Created by Henrik Enblom on 2025-04-21.
//

#include "Buddy.h"
#include "Catalog.h"
#include "System.h"
#include "UI.h"
#include "audio/SIDPlayer.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

Buddy *instance = nullptr;

void Buddy::forceRotationControl() {
    if (lastGestureRequest != RT_G_FORCE_ROTATE) {
        lastGestureRequest = RT_G_FORCE_ROTATE;
        uart_putc(UART_ID, RT_G_FORCE_ROTATE);
    }
}

void Buddy::forceVerticalControl() {
    if (lastGestureRequest != RT_G_FORCE_VERTICAL) {
        lastGestureRequest = RT_G_FORCE_VERTICAL;
        uart_putc(UART_ID, RT_G_FORCE_VERTICAL);
    }
}

void Buddy::enableGestureDetection() {
    if (lastGestureRequest != RT_G_SET_AUTO) {
        lastGestureRequest = RT_G_SET_AUTO;
        uart_putc(UART_ID, RT_G_SET_AUTO);
    }
}

Buddy *Buddy::getInstance() {
    if (instance == nullptr) {
        instance = new Buddy();
    }
    return instance;
}

void buddyCallback() {
    if (uart_is_readable(UART_ID)) {
        auto notificationType = static_cast<NotificationType>(uart_getc(UART_ID));
        if (notificationType == NT_GESTURE) {
            auto gesture = static_cast<Gesture>(uart_getc(UART_ID));
            char flags = uart_getc(UART_ID);
            bool mod1 = IS_BIT_SET(flags, 0);
            switch (gesture) {
                case G_TAP:
                    UI::singleClickCallback(0, nullptr);
                    break;
                case G_DOUBLE_TAP:
                    UI::doubleClickCallback();
                    break;
                case G_HOME:
                    if (Catalog::playlistIsOpen()) {
                        Catalog::getCurrentPlaylist()->resetAccessors();
                    } else {
                        Catalog::goHome();
                    }
                    break;
                case G_VERTICAL:
                    UI::verticalMovement(mod1 ? -1 : 1);
                    break;
                case G_HORIZONTAL:
                    break;
                case G_ROTATE:
                    UI::adjustVolume(mod1);
                    break;
                default: ;
            }
        } else {
            auto buddy = Buddy::getInstance();
            switch (notificationType) {
                case NT_BT_CONNECTING:
                    buddy->setConnecting();
                    break;
                case NT_BT_DISCONNECTED:
                    buddy->setDisconnected();
                    break;
                case NT_BT_DEVICE_LIST_CHANGED:
                    buddy->refreshDeviceList();
                    break;
                case NT_BT_CONNECTED:
                    buddy->setConnected();
                    break;
                default: ;
            }
        }
    }
}

void connectionPinCallback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        Buddy::getInstance()->setConnected();
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        Buddy::getInstance()->setDisconnected();
    }
}

void Buddy::init() {
    gpio_init(BUDDY_ENABLE_PIN);
    gpio_set_dir(BUDDY_ENABLE_PIN, GPIO_OUT);
    gpio_set_drive_strength(BUDDY_ENABLE_PIN, GPIO_DRIVE_STRENGTH_12MA);
    gpio_put(BUDDY_ENABLE_PIN, true);

    gpio_init(BUDDY_BT_CONNECTED_PIN);

    gpio_set_dir(BUDDY_BT_CONNECTED_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(BUDDY_BT_CONNECTED_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true,
                                       connectionPinCallback);
    if (gpio_get(BUDDY_BT_CONNECTED_PIN)) {
        requestAndSetConnectedBTDeviceName();
        setConnected();
    } else {
        reconnectSavedBTDevice();
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
    if (state == READY || state == AWAITING_SELECTION || state == DISCONNECTED) {
        state = REFRESHING;
        char *selectedDevice = nullptr;
        if (!devices.empty()) {
            selectedDevice = devices.at(selectedPosition).name;
            devices.clear();
        }
        uart_set_irq_enables(UART_ID, false, false);
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
        uart_set_irq_enables(UART_ID, true, false);
        state = AWAITING_SELECTION;
    }
}

void Buddy::connectSelected() {
    if (!devices.empty()) {
        selectBTDevice(devices.at(selectedPosition).name);
    }
}

void Buddy::setDisconnected() {
    if (state == CONNECTED) {
        UI::danceFloorStop();
        SIDPlayer::pauseIfPlaying();
    }
    if (state != AWAITING_SELECTION && state != REFRESHING && state != READY) {
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

void Buddy::setConnected() {
    if (state == CONNECTING) {
        requestAndSetConnectedBTDeviceName();
        saveConnectedBTDevice();
    }
    state = CONNECTED;
}

void Buddy::askToDisconnect() {
    state = AWAITING_DISCONNECT_CONFIRMATION;
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
        if (c > UART_READABLE_TIMEOUT_MS) {
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
            if (ch == '\n') {
                buffer[i] = '\0';
                break;
            }
            buffer[i] = ch;
        }
        return true;
    }
    return false;
}

void Buddy::requestAndSetConnectedBTDeviceName() {
    uart_putc(UART_ID, RT_BT_GET_CONNECTED);
    readBTDeviceName(lastConnectedDeviceName);
    if (awaitUartReadable()) {
        for (unsigned char &i: lastConnectedAddr) {
            const uint8_t ch = uart_getc(UART_ID);
            i = ch;
        }
    }
}

void Buddy::requestBTList() {
    uart_putc(UART_ID, RT_BT_LIST);
}

bool Buddy::selectBTDevice(const char *deviceName) {
    selectedDeviceName = deviceName;
    uart_putc(UART_ID, RT_BT_SELECT);
    uart_puts(UART_ID, deviceName);
    state = AWAITING_STATE_CHANGE;
    return true;
}

bool Buddy::loadLastConnectedBTDevice() {
    FIL fil;
    bool result = false;
    if (System::openSettingsFile(&fil, LAST_BT_DEVICE_FILE)) {
        if (f_gets(lastConnectedDeviceName, sizeof(lastConnectedDeviceName), &fil) != nullptr) {
            char lastAddrHex[21];
            f_gets(lastAddrHex, 21, &fil);
            lastConnectedDeviceName[sizeof(lastConnectedDeviceName) - 1] = '\0';
            sscanf(lastAddrHex, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
                   &lastConnectedAddr[0], &lastConnectedAddr[1], &lastConnectedAddr[2],
                   &lastConnectedAddr[3], &lastConnectedAddr[4], &lastConnectedAddr[5]);
            result = true;
        }
        f_close(&fil);
    }
    return result;
}

void Buddy::saveConnectedBTDevice() {
    FIL fil;
    System::openSettingsFile(&fil, LAST_BT_DEVICE_FILE);
    f_printf(&fil, "%s\n", lastConnectedDeviceName);
    f_printf(&fil, "%02X:%02X:%02X:%02X:%02X:%02X\n",
             lastConnectedAddr[0], lastConnectedAddr[1], lastConnectedAddr[2],
             lastConnectedAddr[3], lastConnectedAddr[4], lastConnectedAddr[5]);
    f_close(&fil);
}

bool Buddy::reconnectSavedBTDevice() {
    if (loadLastConnectedBTDevice()) {
        uart_putc(UART_ID, RT_BT_SELECT);
        uart_puts(UART_ID, lastConnectedDeviceName);
        state = AWAITING_STATE_CHANGE;
        return true;
    }
    return false;
}

void Buddy::disconnect() {
    uart_putc(UART_ID, RT_BT_DISCONNECT);
    System::deleteSettingsFile(LAST_BT_DEVICE_FILE);
    state = DISCONNECTED;
    refreshDeviceList();
}
