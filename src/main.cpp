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


void runPossibleSecondWakeUp() {
    if (watchdog_caused_reboot()) {
        int c = 0;
        while (c++ < LONG_PRESS_DURATION_MS) {
            if (gpio_get(ENC_SW_PIN)) {
                System::goDormant();
            }
            busy_wait_ms(1);
        }
    }
}

bool awaitButtonRelease() {
    int c = 0;
    while (!gpio_get(ENC_SW_PIN)) {
        busy_wait_ms(1);
        c++;
    }
    return c >= LONG_PRESS_DURATION_MS;
}

[[noreturn]] int main() {
    set_sys_clock_khz(CLOCK_SPEED_KHZ, true);
    stdio_init_all();
    UI::initUI();
    runPossibleSecondWakeUp();
    UI::screenOn();
    UI::showSplash();
    const bool quickStart = awaitButtonRelease();
    System::enableUsb();
    if (!System::usbConnected()) {
        System::initBuddy();
        Catalog::refresh();
        UI::start(quickStart);
        SIDPlayer::initAudio();
    }
    while (true) {
        UI::updateUI();
    }
}
#pragma clang diagnostic pop
