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
    SIDPlayer::ampOff();
    UI::stop();
    f_unmount("");
    connected = true;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    System::softReset();
}

void System::configureClocks() {
    clocks_hw->resus.ctrl = 0;
    xosc_init();
    pll_init(pll_sys, 1, 1500 * MHZ, 6, 2);
    pll_init(pll_sys, 1, 1500 * MHZ, 6, 2);
    pll_init(pll_usb, 1, 1200 * MHZ, 5, 5);
    clock_configure(clk_ref,
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                    0,
                    12 * MHZ,
                    12 * MHZ);
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    125 * MHZ,
                    125 * MHZ);
    clock_configure(clk_usb,
                    0,
                    CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);
    clock_configure(clk_adc,
                    0,
                    CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);
    clock_configure(clk_rtc,
                    0,
                    CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    46875);
    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    125 * MHZ,
                    125 * MHZ);
    stdio_init_all();
}

void System::softReset() {
    AIRCR_Register = SYSRESETREQ;
}

void System::hardReset() {
    watchdog_enable(1, true);
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
