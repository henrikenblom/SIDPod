#include "device/usbd.h"
#include "ff.h"
#include "PSIDCatalog.h"
#include "audio/SIDPlayer.h"

using namespace std;

extern "C" void filesystem_init();
extern "C" void set_msc_ready_to_attach();

void playFirstSong() {
    PSIDCatalog::refreshCatalog();
    SIDPlayer::loadPSID(PSIDCatalog::getNextPsidFile());
    SIDPlayer::play();
}

void tud_mount_cb(void) {
    f_unmount("");
    SIDPlayer::stop();
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    playFirstSong();
    set_msc_ready_to_attach();
}

int main() {
    filesystem_init();
    SIDPlayer::initAudio();
    playFirstSong();
    tud_init(BOARD_TUD_RHPORT);

    while (true) {
        tud_task();
    }
}