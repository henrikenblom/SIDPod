#include <pico/stdio.h>
#include <pico/time.h>
#include "device/usbd.h"
#include "ff.h"
#include "PSIDCatalog.h"
#include "audio/SIDPlayer.h"
#include "UI.h"

using namespace std;

extern "C" void filesystem_init();
extern "C" void set_msc_ready_to_attach();

struct repeating_timer tudTaskTimer{};
bool connected = false;

void tud_mount_cb(void) {
    connected = true;
    printf("mount\n");
    f_unmount("");
    SIDPlayer::stop();
    UI::stop();
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    connected = false;
    PSIDCatalog::refresh();
    UI::start();
    set_msc_ready_to_attach();
}

bool repeatingTudTask(struct repeating_timer *t) {
    (void) t;
    tud_task();
    return true;
}

void initUsb() {
    tud_init(BOARD_TUD_RHPORT);
    for (int i = 0; i < 1000000; i++) {
        tud_task();
    }
    printf("After initial check\n");
    add_repeating_timer_ms(1, repeatingTudTask, nullptr, &tudTaskTimer);
}

int main() {
    stdio_init_all();
    UI::initUI();
    UI::stop();
    filesystem_init();
    SIDPlayer::initAudio();
    initUsb();
    if (!connected) {
        PSIDCatalog::refresh();
        UI::start();
    }
    while (true) {
        UI::showUI();
    }
}