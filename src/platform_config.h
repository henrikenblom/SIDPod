#pragma once

#define IS_BIT_SET(value, bit) (((value) & (1 << (bit))) != 0)

#ifndef USE_SDCARD
#include "hardware/flash.h"

#ifdef SOLDERPARTY_RP2040_STAMP
#define FLASH_STORAGE_BYTES (7552 * 1024)
#endif
#ifdef RASPBERRYPI_PICO
#define FLASH_STORAGE_BYTES                 (1408 * 1024)
#endif

#define SECTOR_COUNT                        (FLASH_STORAGE_BYTES / FLASH_SECTOR_SIZE)
#define FLASH_BASE_ADDR                     (PICO_FLASH_SIZE_BYTES - FLASH_STORAGE_BYTES)
#define FLASH_MMAP_ADDR                     (XIP_BASE + FLASH_BASE_ADDR)

#endif

#define AIRCR_Register                      (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))
#define SYSRESETREQ                         0x5FA0004

#if defined SYS_CLK_MHZ && SYS_CLK_MHZ == 200
#define CLOCK_SPEED_KHZ                     200000
#else
#define CLOCK_SPEED_KHZ                     125000
#endif

#define SYNC_INTERVAL_MS                    40

#define BOARD_TUD_RHPORT                    0

#ifndef HW_USB_VID
#define HW_USB_VID                          (0x2E8A)
#endif
#ifndef HW_USB_PID
#define HW_USB_PID                          (0x0006)
#endif

#define FS_LABEL                            "SIDPOD"

#define AUDIO_RENDERING_STARTED_FIFO_FLAG   124

#define SID_HEADER_SIZE                    ((uint8_t) 0x7e)
#define SID_MINIMAL_HEADER_SIZE            ((uint8_t) 0x36)
#define PSID_ID                             0x50534944
#define RSID_ID                             0x52534944
#define SID_LOAD_BUFFER_SIZE                ((int) 256)
#define CPU_JSR_WATCHDOG_ABORT_LIMIT        0xffff

#define SAMPLE_RATE                         ((uint32_t)44100)
#define MAX_SAMPLES_PER_BUFFER              880
#define PAL_SPEED_FACTOR                    1.0f
#define NTSC_SPEED_FACTOR                   0.96f
#define NTSC_CPU_FREQUENCY                  1022730
#define PAL_CPU_FREQUENCY                   985248

#define AMP_CONTROL_PIN                     15

#define VOLUME_STEPS                        48
#define INITIAL_VOLUME                      16
#define SONG_SKIP_TIME_MS                   5000

#define I2C_BAUDRATE                        400000
#define DISPLAY_I2C_BLOCK                   i2c1
#define DISPLAY_GPIO_BASE_PIN               2
#define DISPLAY_EXTERNAL_VCC                0
#define DISPLAY_WIDTH                       128
#define DISPLAY_HEIGHT                      32
#define DISPLAY_X_CENTER                    (DISPLAY_WIDTH / 2)
#define DISPLAY_Y_CENTER                    (DISPLAY_HEIGHT / 2)
#define DISPLAY_I2C_ADDRESS                 0x3C

#define UART_ID                             uart1
#define BAUD_RATE                           115200
#define UART_TX_PIN                         8
#define UART_RX_PIN                         9
#define DATA_BITS                           8
#define STOP_BITS                           1
#define PARITY                              UART_PARITY_NONE

#define DISPLAY_STATE_CHANGE_DELAY_MS       500
#define FONT_WIDTH                          6
#define FONT_HEIGHT                         8
#define CATALOG_WINDOW_SIZE                 ((DISPLAY_HEIGHT / FONT_HEIGHT) - 1)
#define MAX_LIST_ENTRIES                    256
#define SONG_LIST_LEFT_MARGIN               6
#define NOW_PLAYING_SYMBOL_HEIGHT           5
#define NOW_PLAYING_SYMBOL_ANIMATION_SPEED  ((float) 0.18)
#define DEFAULT_SPECTRUM_COMPENSATION       ((double ) 0.00002)
#define LINE_LEVEL_SPECTRUM_COMPENSATION    ((double ) 0.00000003)
#define HORIZONTAL_LANDSCAPE_LINES          (DISPLAY_HEIGHT / 2)
#define SOUND_SPRITE_COUNT                  (DISPLAY_HEIGHT * DISPLAY_WIDTH / 8)
#define ROUND_SPRITE_COUNT                  (DISPLAY_WIDTH / 2)
#define STARFIELD_SPRITE_COUNT              (int (DISPLAY_WIDTH * 3))
#define SPECTRUM_SCENE_DURATION_MS          62000
#define ALTERNATIVE_SCENE_DURATION          79000
#define STARFIELD_ACTIVE_AFTER              30000
#define END_SPHERE_SCENE_AFTER              35000
#define FROM_ALTERNATIVE_TRANSITION_DURATION       8600
#define SCROLL_LIMIT                        (-1000)
#define FFT_SAMPLES                         1000
#define LOW_FREQ_DOMINANCE_COMP_OFFSET      2
#define SIDPLAYER_STARTUP_GRACE_TIME        800
#define SONG_NUMBER_DISPLAY_DURATION        3000
#define SONG_NUMBER_SHOW_HIDE_DURATION      1000
#define SONG_NUMBER_DISPLAY_DELAY           1000

#define USER_CONTROLS_POLLRATE_MS           50

#if (!USE_BUDDY)
#define ENC_PIO                             pio1
#define ENC_SM                              1
#define ENC_BASE_PIN                        10
#else
#define SCRIBBLE_TIMEOUT_MS                 500
#endif

#define SWITCH_PIN                          12
#define DOUBLE_CLICK_SPEED_MS               250
#define LONG_PRESS_DURATION_MS              1000
#define DORMANT_ADDITIONAL_DURATION_MS      1500
#define VOLUME_CONTROL_DISPLAY_TIMEOUT      400
#define SPLASH_DISPLAY_DURATION             2000

#define SETTINGS_DIRECTORY                  ".sidpod"
