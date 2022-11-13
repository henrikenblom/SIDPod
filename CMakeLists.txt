cmake_minimum_required(VERSION 3.5)

include(bin2h.cmake)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

set(TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)
set(PICO_SSD1306_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/pico-ssd1306)
set(PICO_BOARD solderparty_rp2040_stamp)
set(BOARD pico_sdk)
set(FAMILY rp2040)
set(CMAKE_C_STANDARD 11)

project(SIDPod)

pico_sdk_init()

add_custom_target(build-time-make-directory ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory cmake-build-debug)

bin2h(SOURCE_FILE resources/kernal HEADER_FILE cmake-build-debug/generated/kernal.h VARIABLE_NAME KERNAL NULL_TERMINATE)

add_executable(${PROJECT_NAME}
        src/main.c
        src/psid_catalog.c
        src/io/ffsystem.c
        src/io/msc_disk.c
        src/io/usb_descriptors.c
        src/io/diskio.c
        src/io/ff_util.c
        src/io/ffunicode.c
        src/io/ff.c
        src/sid/sid.c
        src/visualization/kiss_fftr.c
        src/visualization/kiss_fft.c
        ${PICO_SSD1306_PATH}/ssd1306.c
        )

target_link_libraries(${PROJECT_NAME} PUBLIC
        hardware_rtc
        hardware_sync
        hardware_flash
        hardware_dma
        hardware_adc
        hardware_i2c
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
        ${CMAKE_CURRENT_LIST_DIR}/src/sid/include/
        ${CMAKE_CURRENT_LIST_DIR}/src/io/
        ${CMAKE_CURRENT_LIST_DIR}/src/io/include/
        ${CMAKE_CURRENT_LIST_DIR}/cmake-build-debug/generated/
        ${TINYUSB_PATH}/src/
        ${TINYUSB_PATH}/src/common/
        ${TINYUSB_PATH}/hw/
        ${PICO_SSD1306_PATH}/
        )

target_compile_definitions(${PROJECT_NAME} PUBLIC
        CFG_TUD_MSC=1
        PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
        )

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})