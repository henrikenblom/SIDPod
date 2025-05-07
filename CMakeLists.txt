cmake_minimum_required(VERSION 3.13...3.27)

set(PICO_BOARD solderparty_rp2040_stamp)
#set(PICO_BOARD pico)
#set(PICO_BOARD pico_w)
set(BOARD pico_sdk)
set(FAMILY rp2040)

set(CMAKE_C_STANDARD 11)

set(TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)
#set(USE_SDCARD 1)
#set(USE_BUDDY 1)

include(bin2h.cmake)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

add_compile_definitions(SYS_CLK_MHZ=200)

project(SIDPod)

pico_sdk_init()

bin2h(SOURCE_FILE resources/sidpod.bmp HEADER_FILE cmake-build-debug/generated/sidpod_bmp.h VARIABLE_NAME SIDPOD_BMP NULL_TERMINATE)
bin2h(SOURCE_FILE resources/sidpod_24px_height.bmp HEADER_FILE cmake-build-debug/generated/sidpod_24px_height_bmp.h VARIABLE_NAME SIDPOD_24H_BMP NULL_TERMINATE)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/io/msc_disk.c
        src/io/usb_descriptors.c
        src/audio/c64.cpp
        src/audio/c64.h
        src/visualization/kiss_fftr.c
        src/visualization/kiss_fft.c
        src/display/ssd1306.c
        src/Playlist.cpp
        src/audio/SIDPlayer.cpp
        src/UI.cpp
        src/visualization/DanceFloor.cpp
        src/System.cpp
        src/audio/reSID/envelope.cc
        src/audio/reSID/pot.cc
        src/audio/reSID/voice.cc
        src/audio/reSID/sid.cc
        src/audio/reSID/filter.cc
        src/audio/reSID/extfilt.cc
        src/audio/reSID/wave.cc
        src/audio/reSID/version.cc
        src/audio/reSID/siddefs.h
        src/audio/reSID/extfilt.h
        src/audio/reSID/spline.h
        src/audio/reSID/pot.h
        src/audio/reSID/voice.h
        src/audio/reSID/envelope.h
        src/audio/reSID/wave.h
        src/audio/reSID/sid.h
        src/audio/reSID/filter.h
        src/audio/reSID/filter6581.h
        src/audio/reSID/filter8580.h
        src/audio/sidendian.h
        src/Catalog.cpp
        src/io/flash/diskio.c
        src/io/flash/ff.c
        src/io/flash/ffunicode.c
        src/io/flash/ffsystem.c
        src/io/flash/ff_util.c
        src/io/flash/ff_util.h
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)

target_link_libraries(${PROJECT_NAME}
        hardware_rtc
        hardware_sync
        hardware_dma
        hardware_adc
        hardware_i2c
        hardware_clocks
        hardware_sleep
        hardware_interp
        pico_stdlib
        pico_audio_i2s
        pico_multicore
        pico_unique_id
        tinyusb_common
        tinyusb_device
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/
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
        PICO_AUDIO_I2S_DATA_PIN=26
        PICO_AUDIO_I2S_CLOCK_PIN_BASE=27
)

if (DEFINED USE_SDCARD)
    message("Using SD Card for internal storage")
    if (NOT DEFINED ENV{NO_OS_FFSD_PATH})
        message(FATAL_ERROR "Please clone and set the NO_OS_FFSD_PATH variable to the root of carl3k's \
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico library. \
        It's available at: https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico")
    endif ()
    add_subdirectory($ENV{NO_OS_FFSD_PATH}/src/ build)
    target_link_libraries(${PROJECT_NAME} no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)
    target_sources(${PROJECT_NAME} PRIVATE
            src/io/hw_config.c
    )
    target_compile_definitions(${PROJECT_NAME} PUBLIC
            USE_SDCARD=1
    )
else ()
    message("Using SPI Flash for internal storage")
    target_link_libraries(${PROJECT_NAME} hardware_flash)
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/src/io/flash/
    )
endif ()

if (DEFINED USE_BUDDY)
    message("Using Buddy")
    target_sources(${PROJECT_NAME} PRIVATE
            src/buddy/Buddy.cpp
    )
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/src/buddy/
    )
    target_compile_definitions(${PROJECT_NAME} PUBLIC
            USE_BUDDY=1
    )
endif ()

    set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS
            "-Wl,--print-memory-usage"
    )

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})