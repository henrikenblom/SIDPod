#include <pico/time.h>
#include "device/usbd.h"
#include "ff.h"
#include "PSIDCatalog.h"
#include "audio/SIDPlayer.h"
#include "UI.h"
#include "System.h"
#include <pico/multicore.h>

using namespace std;

extern "C" void filesystem_init();

struct repeating_timer tudTaskTimer{};
bool connected = false;

void tud_mount_cb(void) {
    multicore_reset_core1();
    SIDPlayer::ampOff();
    UI::stop();
    f_unmount("");
    connected = true;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    System::softReset();
}

bool repeatingTudTask(struct repeating_timer *t) {
    (void) t;
    tud_task();
    return true;
}

void initUsb() {
    tud_init(BOARD_TUD_RHPORT);
    for (int i = 0; i < SPLASH_DISPLAY_DURATION * 1000; i++) {
        tud_task();
    }
    add_repeating_timer_ms(1, repeatingTudTask, nullptr, &tudTaskTimer);
}

int main() {
    System::configureClock();
    UI::initUI();
    UI::screenOn();
    UI::showSplash();
    filesystem_init();
    initUsb();
    if (!connected) {
        PSIDCatalog::refresh();
        UI::start();
        SIDPlayer::initAudio();
    }
    while (true) {
        UI::showUI();
    }
}