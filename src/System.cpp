#include <hardware/clocks.h>
#include <device/usbd.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include <pico/sleep.h>
#include "System.h"

#include <set>

#include "UI.h"
#include "platform_config.h"
#include "audio/SIDPlayer.h"
#include "BuddyInterface.h"

repeating_timer tudTaskTimer{};

bool connected = false;

std::set<char[32]> btDeviceList;

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
    gpio_pull_up(ENC_SW_PIN);
    sleep_goto_dormant_until_pin(ENC_SW_PIN, true, false);
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
        btDeviceList.insert(buffer);
    }
}

bool System::selectBTDevice(const char *deviceName) {
    uart_putc(UART_ID, RT_BT_SELECT);
    uart_puts(UART_ID, deviceName);
    return true; //TODO: Check if a device is connected through the use of GPIO
}

void System::initBuddy() {
    //TODO: Connect the ESP_EN pin to some GPIO pin and set it to high

    uart_init(UART_ID, 2400);

    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    uart_set_baudrate(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);
}
