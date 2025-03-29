#pragma once
#include "hardware/flash.h"

#ifdef SOLDERPARTY_RP2040_STAMP
#define FLASH_STORAGE_BYTES (7552 * 1024)
#endif
#ifdef RASPBERRYPI_PICO
#define FLASH_STORAGE_BYTES                 (1408 * 1024)
#endif

#define AIRCR_Register                      (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))
#define SYSRESETREQ                         0x5FA0004

#define SECTOR_COUNT                        (FLASH_STORAGE_BYTES / FLASH_SECTOR_SIZE)
#define FLASH_BASE_ADDR                     (PICO_FLASH_SIZE_BYTES - FLASH_STORAGE_BYTES)
#define FLASH_MMAP_ADDR                     (XIP_BASE + FLASH_BASE_ADDR)

#define FS_LABEL                            "SIDPOD"

#define BOARD_TUD_RHPORT                    0

#ifndef HW_USB_VID
#define HW_USB_VID                          (0x2E8A)
#endif
#ifndef HW_USB_PID
#define HW_USB_PID                          (0x0006)
#endif

#define AUDIO_RENDERING_STARTED_FIFO_FLAG   124

#define PSID_HEADER_SIZE                    ((uint8_t) 0x88)
#define PSID_MINIMAL_HEADER_SIZE            ((uint8_t) 0x48)
#define PSID_ID                             0x50534944
#define RSID_ID                             0x52534944
#define SID_LOAD_BUFFER_SIZE                ((int) 1024)
#define CPU_JSR_WATCHDOG_ABORT_LIMIT        0xffff

#define SAMPLE_RATE                         ((uint32_t)48000)
#define SAMPLES_PER_BUFFER                  1000

#define AMP_CONTROL_PIN                     15

#define VOLUME_STEPS                        20

#define I2C_BAUDRATE                        400000
#define DISPLAY_I2C_BLOCK                   i2c1
#define DISPLAY_GPIO_BASE_PIN               2
#define DISPLAY_EXTERNAL_VCC                0
#define DISPLAY_WIDTH                       128
#define DISPLAY_HEIGHT                      32
#define DISPLAY_I2C_ADDRESS                 0x3C

#define DISPLAY_STATE_CHANGE_DELAY_MS       500
#define FONT_WIDTH                          6
#define FONT_HEIGHT                         8
#define CATALOG_WINDOW_SIZE                 (DISPLAY_HEIGHT / FONT_HEIGHT)
#define SONG_LIST_LEFT_MARGIN               6
#define NOW_PLAYING_SYMBOL_HEIGHT           5
#define NOW_PLAYING_SYMBOL_ANIMATION_SPEED  ((float) 0.18)
#define DEFAULT_SPECTRUM_COMPENSATION       ((double ) 0.00004)
#define LINE_LEVEL_SPECTRUM_COMPENSATION    ((double ) 0.00000003)
#define HORIZONTAL_LANDSCAPE_LINES          (DISPLAY_HEIGHT / 2)
#define SOUND_SPRITE_COUNT                  (DISPLAY_HEIGHT * DISPLAY_WIDTH)
#define FFT_SAMPLES                         SAMPLES_PER_BUFFER
#define SIDPLAYER_STARTUP_GRACE_TIME        200

#define USER_CONTROLS_POLLRATE_MS           50

#define ENC_PIO                             pio1
#define ENC_SM                              1
#define ENC_BASE_PIN                        10
#define ENC_SW_PIN                          12
#define DOUBLE_CLICK_SPEED_MS               250
#define LONG_PRESS_DURATION_MS              1000
#define DORMANT_ADDITIONAL_DURATION_MS      1500
#define VOLUME_CONTROL_DISPLAY_TIMEOUT      1400
#define SPLASH_DISPLAY_DURATION             2500