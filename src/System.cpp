#include <hardware/pll.h>
#include <hardware/clocks.h>
#include <device/usbd.h>
#include <pico/multicore.h>
#include <hardware/xosc.h>
#include <hardware/watchdog.h>
#include <pico/sleep.h>
#include "System.h"
#include "UI.h"
#include "platform_config.h"
#include "audio/SIDPlayer.h"


repeating_timer tudTaskTimer{};

bool connected = false;

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
    add_repeating_timer_ms(1, repeatingTudTask, nullptr, &tudTaskTimer);
}

bool System::repeatingTudTask(struct repeating_timer *t) {
    (void) t;
    tud_task();
    return true;
}

bool System::usbConnected() {
    return connected;
}
