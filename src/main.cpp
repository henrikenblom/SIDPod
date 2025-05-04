// ReSharper disable once CppUnusedIncludeDirective
#include <hardware/clocks.h>
#include <device/usbd.h>
#include <hardware/watchdog.h>
#include "platform_config.h"
#include "audio/SIDPlayer.h"
#include "UI.h"
#include "System.h"
#include "Catalog.h"

using namespace std;

void buddyCallback();

void runPossibleSecondWakeUp() {
    if (watchdog_caused_reboot()) {
        int c = 0;
        while (c++ < LONG_PRESS_DURATION_MS) {
            if (gpio_get(SWITCH_PIN)) {
                System::goDormant();
            }
            busy_wait_ms(1);
        }
    }
}

bool awaitButtonRelease() {
    int c = 0;
    while (!gpio_get(SWITCH_PIN)) {
        busy_wait_ms(1);
        c++;
    }
    return c >= LONG_PRESS_DURATION_MS;
}

#if USE_BUDDY
void initUart() {
    uart_init(UART_ID, 2400);

    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    uart_set_baudrate(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

    irq_set_exclusive_handler(UART1_IRQ, buddyCallback);
    irq_set_enabled(UART1_IRQ, true);

    uart_set_irq_enables(UART_ID, true, false);
}
#endif

[[noreturn]] int main() {
    set_sys_clock_khz(CLOCK_SPEED_KHZ, true);
    stdio_init_all();
#if USE_BUDDY
    initUart();
#endif
    UI::initUI();
    runPossibleSecondWakeUp();
    UI::screenOn();
    UI::showSplash();
    const bool quickStart = awaitButtonRelease();
    System::enableUsb();
    if (!System::usbConnected()) {
        System::prepareFilesystem();
        Catalog::refresh();
        UI::start(quickStart);
        SIDPlayer::initAudio();
    }
    while (true) {
        UI::updateUI();
    }
}
#pragma clang diagnostic pop
