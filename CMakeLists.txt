cmake_minimum_required(VERSION 3.5)

#set(PICO_BOARD solderparty_rp2040_stamp)
#set(PICO_BOARD pico)
set(PICO_BOARD pico_w)
set(BOARD pico_sdk)
set(FAMILY rp2040)

set(CMAKE_C_STANDARD 11)

set(TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)

include(bin2h.cmake)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

#add_compile_definitions(SYS_CLK_MHZ=200)

project(SIDPod)

pico_sdk_init()

bin2h(SOURCE_FILE resources/sidpod.bmp HEADER_FILE cmake-build-debug/generated/sidpod_bmp.h VARIABLE_NAME SIDPOD_BMP NULL_TERMINATE)
bin2h(SOURCE_FILE resources/sidpod_24px_height.bmp HEADER_FILE cmake-build-debug/generated/sidpod_24px_height_bmp.h VARIABLE_NAME SIDPOD_24H_BMP NULL_TERMINATE)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/io/ffsystem.c
        src/io/msc_disk.c
        src/io/usb_descriptors.c
        src/io/diskio.c
        src/io/ff_util.c
        src/io/ffunicode.c
        src/io/ff.c
        src/visualization/kiss_fftr.c
        src/visualization/kiss_fft.c
        src/display/ssd1306.c
        src/PSIDCatalog.cpp
        src/audio/SIDPlayer.cpp
        src/UI.cpp
        src/visualization/DanceFloor.cpp
        src/System.cpp
        src/audio/libsidplayfp/src/builders/resid-builder/resid/envelope.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave8580_P_T.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/pot.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/voice.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/sid.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/filter.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave6581_P_T.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave6581_PS_.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave8580_PST.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave8580__ST.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave6581_PST.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave8580_PS_.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave6581__ST.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/extfilt.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/wave.cc
        src/audio/libsidplayfp/src/builders/resid-builder/resid/version.cc
        src/audio/libsidplayfp/src/EventScheduler.cpp
        src/audio/libsidplayfp/src/psiddrv.cpp
        src/audio/libsidplayfp/src/reloc65.cpp
        src/audio/libsidplayfp/src/c64/VIC_II/mos656x.cpp
        src/audio/libsidplayfp/src/c64/CPU/mos6510.cpp
        src/audio/libsidplayfp/src/c64/CPU/mos6510debug.cpp
        src/audio/libsidplayfp/src/c64/c64.cpp
        src/audio/libsidplayfp/src/c64/mmu.cpp
        src/audio/libsidplayfp/src/c64/CIA/mos652x.cpp
        src/audio/libsidplayfp/src/c64/CIA/tod.cpp
        src/audio/libsidplayfp/src/c64/CIA/SerialPort.cpp
        src/audio/libsidplayfp/src/c64/CIA/interrupt.cpp
        src/audio/libsidplayfp/src/c64/CIA/timer.cpp
        src/audio/libsidplayfp/src/player.cpp
        src/audio/libsidplayfp/src/builders/resid-builder/resid-builder.cpp
        src/audio/libsidplayfp/src/builders/resid-builder/resid-emu.cpp
        src/audio/libsidplayfp/src/sidtune/PSID.cpp
        src/audio/libsidplayfp/src/sidtune/SidTuneBase.cpp
        src/audio/libsidplayfp/src/sidtune/MUS.cpp
        src/audio/libsidplayfp/src/sidtune/SidTuneTools.cpp
        src/audio/libsidplayfp/src/sidtune/prg.cpp
        src/audio/libsidplayfp/src/sidtune/p00.cpp
        src/audio/libsidplayfp/src/sidemu.cpp
        src/audio/libsidplayfp/src/mixer.cpp
        src/audio/libsidplayfp/src/utils/md5Factory.cpp
        src/audio/libsidplayfp/src/utils/MD5/MD5.cpp
        src/audio/libsidplayfp/src/utils/STILview/stil.cpp
        src/audio/libsidplayfp/src/EventScheduler.cpp
        src/audio/libsidplayfp/src/sidplayfp/SidConfig.cpp
        src/audio/libsidplayfp/src/sidplayfp/sidplayfp.cpp
        src/audio/libsidplayfp/src/sidplayfp/SidInfo.cpp
        src/audio/libsidplayfp/src/sidplayfp/sidbuilder.cpp
        src/audio/libsidplayfp/src/sidplayfp/SidTune.cpp
        src/audio/libsidplayfp/src/sidplayfp/SidTuneInfo.cpp
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)

target_link_libraries(${PROJECT_NAME}
        hardware_rtc
        hardware_sync
        hardware_flash
        hardware_dma
        hardware_adc
        hardware_i2c
        hardware_clocks
        hardware_sleep
        pico_stdlib
        pico_audio_i2s
        pico_multicore
        pico_unique_id
        tinyusb_common
        tinyusb_device
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/builders/resid-builder/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/builders/resid-builder/resid/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/builders/resid-builder/resid/6581/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/builders/resid-builder/resid/8580/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/sidtune/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/sidtune/PSID/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/sidtune/MUS/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/sidtune/p00/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/sidtune/prg/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/sidtune/STILview/
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/libsidplayfp/src/sidtune/
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

set(BTSTACK_3RD_PARTY_PATH ${PICO_SDK_PATH}/lib/btstack/3rd-party)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        CFG_TUD_MSC=1
        PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
        PICO_AUDIO_I2S_MONO_INPUT=1
        PICO_AUDIO_I2S_DATA_PIN=26
        PICO_AUDIO_I2S_CLOCK_PIN_BASE=27
        PICO_AUDIO_I2S_MONO_OUTPUT=1
)

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})