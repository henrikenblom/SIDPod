#include <pico/sleep.h>
#include <hardware/pll.h>
#include <hardware/clocks.h>
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

void System::goDormant() {
    UI::screenOff();
    SIDPlayer::turnAmpOff();
    sleep_run_from_xosc();
    sleep_goto_dormant_until_pin(ENC_SW_PIN, true, false);
    configureClock();
    UI::screenOn();
    SIDPlayer::turnAmpOn();
}

void System::softReset() {
    AIRCR_Register = SYSRESETREQ;
}