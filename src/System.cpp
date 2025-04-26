#include <device/usbd.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include <pico/sleep.h>
#include "System.h"

#include "Buddy.h"
#include "UI.h"
#include "platform_config.h"
#include "audio/SIDPlayer.h"
#include "Catalog.h"
#include "sd_card.h"

repeating_timer tudTaskTimer{};

bool connected = false;
bool mounted = false;

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

void System::deleteSettingsFile(const char *fileName) {
    TCHAR fullPath[FF_LFN_BUF + 1];
    snprintf(fullPath, FF_LFN_BUF + 1, "%s/%s", SETTINGS_DIRECTORY, fileName);
    f_unlink(fullPath);
}

bool System::openSettingsFile(FIL *fil, const char *fileName) {
    if (!mounted) {
        mountAndPrepareFilesystem();
    }
    TCHAR fullPath[FF_LFN_BUF + 1];
    snprintf(fullPath, FF_LFN_BUF + 1, "%s/%s", SETTINGS_DIRECTORY, fileName);
    const FRESULT fr = f_open(fil, fullPath, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    return fr == FR_OK;
}

bool System::createSettingsDirectoryIfNotExists() {
    DIR *dp = new DIR;
    FRESULT fr = f_opendir(dp, SETTINGS_DIRECTORY);
    if (fr == FR_OK) {
        f_closedir(dp);
        delete dp;
        return true;
    }
    fr = f_mkdir(SETTINGS_DIRECTORY);
    if (fr != FR_OK) {
        return false;
    }
    return true;
}

bool System::mountAndPrepareFilesystem() {
    sd_card_t *sd_card_p = sd_get_by_drive_prefix("0:");
    FATFS *fs_p = &sd_card_p->state.fatfs;
    f_mount(fs_p, "0:", 1);
    sd_card_p->state.mounted = true;
    createSettingsDirectoryIfNotExists();
    return true;
}
