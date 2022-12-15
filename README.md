![](doc_resources/SIDPod_logo.png)

___

## What is it?

The SIDPod is a PSID player for RP2040 microcontroller based boards, such as the
[Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) or the 
[Solder Party RP2040 Stamp](https://www.solder.party/docs/rp2040-stamp/). Playback is based on the SID module found in
[Rockbox](https://www.rockbox.org/), which in turn is based on TinySID.

## Building

SIDPod requires both the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk/) and the 
[pico-extras](https://github.com/raspberrypi/pico-extras). Make sure to grab these and either put them next to the
SIDPod directory or, if you have them somewhere else, point the `PICO_SDK_PATH` and `PICO_EXTRAS_PATH` environment
variables to their respective location. You'll also need `cmake` on your path. If/when these prerequisites have been
satisfied, you almost ready to run the `cmake` command to create the build directory. The last thing to do is to make
sure the board type matches the one you have. By default, SIDPod builds for the Solder Party RP2040 Stamp, which has a
larger NAND flash than the more common Raspberry Pi Pico. Using a RP2040 Stamp UF2 would most likely cause issues on
the first run, as the flash will get formatted if no readable FAT filesystem is found on it. Changing board type can
either be done by modifying the `PICO_BOARD` variable in [CMakeLists.txt](CMakeLists.txt) or by setting the `PICO_BOARD`
environment variable before running the `cmake` commands. The correct value for the Raspberry Pi Pico is `pico`. Once
this is done, the build directory is created by running:

`cmake -S . -B cmake-build-debug`...or if you have [Ninja](https://ninja-build.org/) installed, you can save a few
seconds of your valuable time by running `cmake -G Ninja -S . -B cmake-build-debug` instead.

Then, to build the `SIDPod.uf2` binary, issue:

`cmake --build cmake-build-debug --target all -j 12`

### Software

### Hardware

#### Bill of materials

## Using

### Finding music

### Adding music

### Playback

### Volume and level control

### Locking

### Deep sleep

## Developing