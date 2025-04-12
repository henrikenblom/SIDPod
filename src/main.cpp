// ReSharper disable once CppUnusedIncludeDirective
#include <hardware/clocks.h>
#include <device/usbd.h>
#include <hardware/watchdog.h>
#include "platform_config.h"
#include "Playlist.h"
#include "audio/SIDPlayer.h"
#include "UI.h"
#include "System.h"
#include "Catalog.h"

using namespace std;

extern "C" void filesystem_init();

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


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main() {
    set_sys_clock_khz(CLOCK_SPEED_KHZ, true);
    stdio_init_all();
    UI::initUI();
    runPossibleSecondWakeUp();
    SIDPlayer::ampOn();
    UI::screenOn();
    UI::showSplash();
    while (!gpio_get(ENC_SW_PIN)) {
    }
    filesystem_init();
    System::enableUsb();
    if (!System::usbConnected()) {
        Catalog::refresh();
        Playlist::refresh();
        UI::start();
        SIDPlayer::initAudio();
    }
    while (true) {
        UI::updateUI();
    }
}
#pragma clang diagnostic pop
