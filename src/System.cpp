#include <pico/sleep.h>
#include <hardware/pll.h>
#include <hardware/clocks.h>
#include <cstdio>
#include "System.h"
#include "UI.h"
#include "platform_config.h"
#include "audio/SIDPlayer.h"

void System::configureClock() {
    pll_init(pll_sys, 1, 1500 * MHZ, 6, 2);
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    125 * MHZ,
                    125 * MHZ);
    stdio_init_all();
}

void System::softReset() {
    AIRCR_Register = SYSRESETREQ;
}

void System::goDormant() {
    UI::powerOffScreenCallback();
    busy_wait_ms(DISPLAY_STATE_CHANGE_DELAY_MS);
    SIDPlayer::ampOff();
    sleep_run_from_xosc();
    sleepUntilDoubleClick();
    configureClock();
}

void System::sleepUntilDoubleClick() {
    sleep_run_from_xosc();
    bool sleep = true;
    while (sleep) {
        sleep_goto_dormant_until_pin(ENC_SW_PIN, true, false);
        gpio_pull_up(ENC_SW_PIN);
        bool lastSwitchState = !gpio_get(ENC_SW_PIN);
        bool inDoubleClickSession = false;
        for (int i = 0; i < DOUBLE_CLICK_SPEED_MS; i++) {
            bool currentSwitchState = !gpio_get(ENC_SW_PIN);
            if (currentSwitchState && currentSwitchState != lastSwitchState) {
                inDoubleClickSession = true;
            } else if (currentSwitchState != lastSwitchState && inDoubleClickSession) {
                sleep = false;
                break;
            }
            lastSwitchState = currentSwitchState;
            busy_wait_ms(1);
        }
    }
    gpio_pull_up(ENC_SW_PIN);
}