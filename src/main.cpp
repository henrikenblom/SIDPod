#include "device/usbd.h"
#include "ff.h"
#include "PSIDCatalog.h"

using namespace std;

extern "C" void filesystem_init();
extern "C" void set_msc_ready_to_attach();

void tud_mount_cb(void) {
    f_unmount("");
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    PSIDCatalog::refreshCatalog();
    set_msc_ready_to_attach();
}

int main() {
    filesystem_init();
    PSIDCatalog::refreshCatalog();
    size_t s = PSIDCatalog::getSize();
    printf("%d", s);

    tud_init(BOARD_TUD_RHPORT);

    while (true) {
        tud_task();
    }
}