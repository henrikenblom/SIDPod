#include "PSIDCatalog.h"
#include "audio/SIDPlayer.h"
#include "UI.h"
#include "System.h"

using namespace std;

extern "C" void filesystem_init();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main() {
    System::configureClocks();
    UI::initUI();
    UI::screenOn();
    UI::showSplash();
    filesystem_init();
    System::enableUsb();
    if (!System::usbConnected()) {
        PSIDCatalog::refresh();
        UI::start();
        SIDPlayer::initAudio();
    }
    while (true) {
        UI::updateUI();
    }
}
#pragma clang diagnostic pop