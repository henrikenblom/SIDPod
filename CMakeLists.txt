cmake_minimum_required(VERSION 3.5)

include(bin2h.cmake)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

set(TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)
set(PICO_BOARD solderparty_rp2040_stamp)
#set(PICO_BOARD pico)
set(BOARD pico_sdk)
set(FAMILY rp2040)
set(CMAKE_C_STANDARD 11)
set(USE_WAVESHARE_PICO_AUDIO 1)

project(SIDPod)

pico_sdk_init()

bin2h(SOURCE_FILE resources/kernal HEADER_FILE cmake-build-debug/generated/kernal.h VARIABLE_NAME KERNAL NULL_TERMINATE)
bin2h(SOURCE_FILE resources/sidpod.bmp HEADER_FILE cmake-build-debug/generated/sidpod_bmp.h VARIABLE_NAME SIDPOD_BMP NULL_TERMINATE)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/io/ffsystem.c
        src/io/msc_disk.c
        src/io/usb_descriptors.c
        src/io/diskio.c
        src/io/ff_util.c
        src/io/ffunicode.c
        src/io/ff.c
        src/audio/sid.c
        src/visualization/kiss_fftr.c
        src/visualization/kiss_fft.c
        src/display/ssd1306.c
        src/PSIDCatalog.cpp src/PSIDCatalog.h src/PSIDCatalogEntry.h src/audio/SIDPlayer.cpp src/audio/SIDPlayer.h src/UI.cpp src/UI.h src/visualization/DanceFloor.cpp src/visualization/DanceFloor.h src/System.cpp src/System.h)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)

target_link_libraries(${PROJECT_NAME} PUBLIC
        hardware_rtc
        hardware_sync
        hardware_flash
        hardware_dma
        hardware_adc
        hardware_i2c
        hardware_clocks
        hardware_sleep
        pico_bootsel_via_double_reset
        pico_stdlib
        pico_audio_i2s
        pico_multicore
        pico_unique_id
        tinyusb_common
        tinyusb_device
        )

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/include/
        ${CMAKE_CURRENT_LIST_DIR}/src/visualization/include/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/include/
        ${CMAKE_CURRENT_LIST_DIR}/src/io/
        ${CMAKE_CURRENT_LIST_DIR}/src/io/include/
        ${CMAKE_CURRENT_LIST_DIR}/src/display/include/
        ${CMAKE_CURRENT_LIST_DIR}/cmake-build-debug/generated/
        ${TINYUSB_PATH}/src/
        ${TINYUSB_PATH}/src/common/
        ${TINYUSB_PATH}/hw/
        )

target_compile_definitions(${PROJECT_NAME} PUBLIC
        CFG_TUD_MSC=1
        PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
        PICO_AUDIO_I2S_MONO_INPUT=1
        )

if (USE_WAVESHARE_PICO_AUDIO)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
            PICO_AUDIO_I2S_DATA_PIN=26
            PICO_AUDIO_I2S_CLOCK_PIN_BASE=27
            )
endif ()

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})