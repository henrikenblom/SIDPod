#include "main.h"
#include "device/usbd.h"
#include "msc_control.h"
#include "psid_catalog.h"
#include "ff_util.h"

void tud_mount_cb(void) {
    f_unmount("");
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    set_msc_ready_to_attach();
    index_psid_files();
}

int main() {
    filesystem_init();
    index_psid_files();

    tud_init(BOARD_TUD_RHPORT);

    while (true) {
        tud_task();
    }
}