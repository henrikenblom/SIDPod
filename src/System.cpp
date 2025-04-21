#include <device/usbd.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include <pico/sleep.h>
#include "System.h"

#include <set>
#include <string>

#include "UI.h"
#include "platform_config.h"
#include "audio/SIDPlayer.h"
#include "BuddyInterface.h"
#include "Catalog.h"

repeating_timer tudTaskTimer{};

bool connected = false;

std::set<std::string> btDeviceList;

void tud_mount_cb() {
    multicore_reset_core1();
    UI::stop();
    f_unmount("");
    connected = true;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    System::softReset();
}

void System::softReset() {
    AIRCR_Register = SYSRESETREQ;
}

void System::hardReset() {
    watchdog_enable(1, true);
}

void System::virtualVBLSync() {
    busy_wait_us(WAIT_SYNC_NS);
}

void System::goDormant() {
    SIDPlayer::ampOff();
    sleep_run_from_rosc();
    gpio_pull_up(SWITCH_PIN);
    sleep_goto_dormant_until_pin(SWITCH_PIN, true, false);
    hardReset();
}

void System::enableUsb() {
    tud_init(BOARD_TUD_RHPORT);
    add_repeating_timer_us(100, repeatingTudTask, nullptr, &tudTaskTimer);
}

bool System::repeatingTudTask(struct repeating_timer *t) {
    (void) t;
    tud_task();
    return true;
}

bool System::usbConnected() {
    return connected;
}

#if USE_BUDDY

bool System::awaitUartReadable() {
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

bool System::readBTDeviceName(char *buffer) {
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

void System::requestBTList() {
    uart_putc(UART_ID, RT_BT_LIST);
    uart_putc(UART_ID, 0);
}

void System::updateBTDeviceList() {
    requestBTList();
    char buffer[32];
    while (readBTDeviceName(buffer)) {
        btDeviceList.insert(std::string(buffer));
    }
}

bool System::selectBTDevice(const char *deviceName) {
    uart_putc(UART_ID, RT_BT_SELECT);
    uart_puts(UART_ID, deviceName);
    return true; //TODO: Check if a device is connected through the use of GPIO
}

void System::buddyCallback(uint gpio, uint32_t events) {
    (void) events;
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
        default:
            break;
    }
}

void System::initBuddy() {
    uart_init(UART_ID, 2400);

    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    uart_set_baudrate(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

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
    gpio_init(BUDDY_BT_CONNECTED_PIN);

    gpio_set_dir(BUDDY_MODIFIER1_PIN, GPIO_IN);
    gpio_set_dir(BUDDY_MODIFIER2_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(BUDDY_TAP_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
    gpio_set_irq_enabled_with_callback(BUDDY_VERTICAL_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
    gpio_set_irq_enabled_with_callback(BUDDY_HORIZONTAL_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
    gpio_set_irq_enabled_with_callback(BUDDY_ROTATE_PIN, GPIO_IRQ_EDGE_RISE, true, buddyCallback);
}

#endif
