![](assets/SIDPod_logo.png)

_The SIDPod is a PSID player for RP2040 microcontroller based boards, such as the
[Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) or the
[Solder Party RP2040 Stamp](https://www.solder.party/docs/rp2040-stamp/). Playback is based on the SID emulator found in
[Teensy-reSID](https://github.com/FrankBoesing/Teensy-reSID), which in turn is based on
[reSID](https://github.com/daglem/reSID)_

___

<!-- TOC -->
  * [Building](#building)
    * [Software](#software)
      * [Building from source](#building-from-source)
      * [Prebuilt binaries](#prebuilt-binaries)
    * [Hardware](#hardware)
      * [Construction](#construction)
      * [Power](#power)
      * [Bill of materials](#bill-of-materials)
  * [Using](#using)
    * [Finding music](#finding-music)
    * [Creating playlists](#creating-playlists)
    * [Adding music](#adding-music)
    * [Playback](#playback)
      * [Chip support](#chip-support)
      * [Playlist selection](#playlist-selection)
      * [Song selection](#song-selection)
    * [Volume control](#volume-control)
    * [Pocket mode](#pocket-mode)
    * [Deep sleep](#deep-sleep)
  * [Beta disclaimer](#beta-disclaimer)
  * [What's on the horizon?](#whats-on-the-horizon)
<!-- TOC -->

## Building

### Software

#### Building from source

SIDPod requires both the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk/) and the
[pico-extras](https://github.com/raspberrypi/pico-extras). Make sure to grab these and either put them next to the
SIDPod directory or, if you have them somewhere else, point the `PICO_SDK_PATH` and `PICO_EXTRAS_PATH` environment
variables to their respective location. You'll also need CMake on your path. If/when these prerequisites have been
satisfied, you're almost ready to run the `cmake` command to create the build directory and configure the build. One
last thing to do is to make sure the board type matches the one you have. By default, SIDPod builds for the Solder Party
RP2040 Stamp, which, for example, has a larger NAND flash than the more common Raspberry Pi Pico. Using a RP2040 Stamp
UF2 on a Pi Pico would most likely cause issues on the first run, as the flash will get formatted, **using board
specific settings**, if no readable FAT filesystem is found on it. Changing board type can either be done by modifying
the `PICO_BOARD` variable in [CMakeLists.txt](CMakeLists.txt) or by setting the `PICO_BOARD` environment variable before
running the `cmake` commands. The correct value for the Raspberry Pi Pico is 'pico'. Once this is done, the build is
configured by running:

`cmake -B cmake-build-debug`

Alternatively, if you have [Ninja](https://ninja-build.org/) installed, you can save a few seconds of your valuable time
by instead running:

`cmake -G Ninja -B cmake-build-debug`

Then, to build the _SIDPod.uf2_ binary, issue:

`cmake --build cmake-build-debug --target all`

Once the build finishes, just follow the official instructions for loading a UF2 binary onto the board. For Raspberry Pi
Pico you hold down the _BOOTSEL_ button when you plug it in and then copy _SIDPod.uf2_ to it, once it appears as a
drive named _RPI-RP2_. For some other types of boards, such as the Solder Party RP2040 Stamp, this mode is activated by
double-clicking the reset button.

Also note that if you're running macOS, copying using Finder might fail with this
error: _The operation can’t be completed because an unexpected error occurred (error code 100093)._
If this is the case, fire up the 'ol terminal and do:

`cp cmake-build-debug/SIDPod.uf2 /Volumes/RPI-RP2`

#### Prebuilt binaries

To get a jump start you can also grab the [prebuilt binaries](https://github.com/henrikenblom/SIDPod/releases/latest).
There are two versions, one for the Solder Party RP2040 Stamp and one for the Raspberry Pi Pico.

### Hardware

#### Construction

Since the main goal is to make a portable player, the actual build and design is up to you, the maker. My current
prototype is built inside a 3-AA battery box I had lying around, using a Solder Party RP2040 Stamp and a 3.7V 150mAh
LiPo battery. However, the breadboard design I'm using for development, based on a Raspberry Pi Pico H, looks like this:

![](assets/SIDPod_bb.png)  
_The breadboard schematics._

In reality, the development breadboard looks a bit different. I use a dual lane breadboard and connect to the SIDPod
Pico board via a [Picoprobe](https://github.com/raspberrypi/picoprobe), for debugging and a less painful development
process.

![](assets/SIDPod_bb_photo.jpg)  
_The development breadboard setup._

![](assets/SIDPod_prototype.jpg)  
_The working prototype. Please excuse the soldering. I'm a lover, not a solderer. 😜_

#### Power

The main advantage of the Solder Party RP2040 Stamp, besides its size, is the onboard LiPo charger. This removes some
complexity and saves space when moving beyond the breadboard design. Many RP2040 projects can easily run off the power
provided from the host computer over the built-in USB port. The SIDPod however, will enter a special USB device state
and start to mimic a USB mass storage device once connected to a computer. [More on that here](#adding-music). This
poses a problem, if you actually want to use it for what it's designed for - to play PSIDs. That problem can easily be
solved by powering it from any power source connected to the _VSYS_ pin. That power source can be anything from a 3.7V
LiPo battery to a separate USB breakout bord, with the breakout boards _VBUS_ connected to _VSYS_ on the Pico Board and
the breakout board GND connector connected to any GND on the Pico Board. In my development breadboard setup I power the
SIDPod Pico Board from the Picoprobe Pico Board. In the next section I've listed the components needed for a few
different options:

#### Bill of materials

_Option 1 - Using a Solder Party RP2040 Stamp (the prototype setup):_

- 1 x [Solder Party RP2040 Stamp](https://www.electrokit.com/en/product/rp2040-stamp/)
- 1 x [Adafruit I2S 3W Class D Amp - MAX98357A](https://www.electrokit.com/en/product/forstarkare-klass-d-3w-mono-i2s/)
- 1 x [LCD OLED 0.91″ 128x32px I2C](https://www.electrokit.com/en/product/lcd-oled-0-91-128x32px-i2c/)
- 1 x 3.5mm stereo jack
- 1 x [Rotary encoder w switch](https://www.electrokit.com/en/product/rotary-encoder-module/)
- 2 x [100Ω resistor](https://www.electrokit.com/en/product/resistor-1w-5-100ohm-100r/)
- 1 x [Battery LiPo 3.7V 150mAh](https://www.electrokit.com/en/product/batteri-lipo-3-7v-150mah/)

_Option 2 - Using a Raspberry Pi Pico (the breadboard setup):_

- 1 x [Adafruit I2S 3W Class D Amp - MAX98357A](https://www.electrokit.com/en/product/forstarkare-klass-d-3w-mono-i2s/)
- 1 x [LCD OLED 0.91″ 128x32px I2C](https://www.electrokit.com/en/product/lcd-oled-0-91-128x32px-i2c/)
- 1 x [3.5mm stereo jack](https://www.electrokit.com/en/product/3-5mm-jack-breakout-2/)
- 1 x [Rotary encoder w switch](https://www.electrokit.com/en/product/rotary-encoder-module/)
- 2 x [100Ω resistor](https://www.electrokit.com/en/product/resistor-1w-5-100ohm-100r/)

_Option 2a - Add a Raspberry Pi Pico with Picoprobe, for debugging, binary upload and power through VSYS:_

- 2 x [Raspberry Pi Pico H](https://www.electrokit.com/en/product/raspberry-pi-pico-h/)

_Option 2b - Power the Raspberry Pi Pico through a USB breakout board:_

- 1 x [Raspberry Pi Pico H](https://www.electrokit.com/en/product/raspberry-pi-pico-h/)
- 1 x [Breakout board USB-C](https://www.electrokit.com/en/product/anslutningskort-usb-c/)

## Using

To keep the construction as simple as possible I tried to minimize the number of physical controls, and a rotary encoder
with a built-in push button provides enough versatility to cater to every possible use case of this rather minimal
design. Therefore, controls are both contextual and pattern driven.

### Finding music

The [SID or Sound Interface Device](https://en.wikipedia.org/wiki/MOS_Technology_6581), which this player does a
rudimentary emulation of, was a component in the world's most popular home computer, the
[Commodore 64](https://en.wikipedia.org/wiki/Commodore_64). Due to its legendary status and, to this day, very active
community, there's no shortage of SID files to fill your SIDPod with. The most complete collection is the
[High Voltage SID Collection](https://www.hvsc.c64.org/). It has everything, from cult classics by the legendary Jeroen
Tel to contemporary pieces by Kamil Wolnikowski, aka Jammer. Download the full archive and use one of the SID players
referenced on that page to find the music you like. The collection contains both PSIDs and RSIDs. Most PSIDs are
playable and with the introduction of reSID, (version 2.0beta and later) some RSIDs are playable. However, full
compatibility is not guaranteed yet. _This is being worked on though._

### Creating playlists

With the introduction of version 2.0beta, **music must be put in playlists**. And the definition of a playlist is a
folder with at least one PSID in it. Furthermore, to avoid out of memory errors, the maximum number of PSIDs in a
playlist is restricted to 256. And the number of playlists is limited to 256 as well. This means that the maximum number
of PSIDs is 65536. Coincidentally, the memory size of our favorite 8-bit home computer. ❤️

_As you may have figured out, this many PSIDs would never fit on the internal flash. They will however fit on a
standard size SD card, which is something that will be supported when SIDPod 2.0 is out of beta._

### Adding music

The SIDPod doubles as a USB mass storage device, which means that if you plug it in to your computer it will show up as
a regular, FAT-formatted drive. This means that adding and removing PSIDs is as easy as copying or deleting files. There
are a few caveats though:

- Since version 2.0beta PSIDs must be put in folders. As mentioned above.
- Use it with moderation. [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) doesn't have any
  [wear levelling algorithm](https://en.wikipedia.org/wiki/Wear_leveling), and NAND flash storage wears out faster than
  regular flash drives. Note though that this only goes for write operations. Reading is totally harmless.
- It's slower than you're probably used to. This is due to several factors. One being that flash memory blocks needs to
  be erased before new data can be written to them.

While the SIDPod is connected, the screen shows flickering bars. Don't be alarmed. This is a feature, not a bug. In
fact, it's a reference to the C64 - those who know, know.

### Playback

#### Chip support

Since the introduction of version 2.0beta, the SIDPod supports both the MOS 6581 and MOS 8580 chip models, and up to
three SID chips. There's no need to worry about which chip is used in the PSID file, or how many, as the SIDPod will
automatically detect this and set up the emulation accordingly.

_Disclaimer: Some 3-SID songs are a bit too much for the poor SIDPod, resulting in partial stutter when the filters are
active. Unless you overclock it! Currently, the clock speed is set to 200MHz, which is the highest officially supported
setting. For songs like Melbourne Shuffle by Jammer, you need to stretch this all the way up to 260. Which might damage
your microcontroller!_

#### Playlist selection

![playlists.png](assets/catalog.png)

Once you've added some funky tunes and **safely disconnected** the SIDPod it presents the playlists. In alphabetical
order. The controls in this mode are:

- Scroll through the list by turning the encoder.
- Single-clicking enters the selected playlist.

If you've started a song, as described in the next section, the relevant playlist is decorated with an animated spectrum
analyser symbol. When a song is paused, the animation is paused as well.

#### Song selection

![playlist.png](assets/playlist.png)

Once you've entered a playlist, the screen shows the songs in that playlist. Also in alphabetical order. There's also a
*<< Return* entry at the top. Use it to navigate back to the playlist selection view. The controls in this mode are:

- Scroll through the list by turning the encoder.
- Single-clicking plays the selected song and enters a very retro-fancy visualization mode.
- Double-clicking plays or pauses the selected song but keeps the playlist mode.
- Double-clicking in visualization mode pauses playback and displays a pause notification on screen.

The currently selected song is decorated with a traditional play symbol and the currently playing song is decorated with
an animated spectrum analyser symbol. When a song is paused, the animation is paused as well.

If a song gets crossed out when you're trying to play it, it means that it's either corrupt or uses a combination of
opcodes that have caused the song loading routine to enter an infinite loop and seemingly hang. This is very rare for
PSIDs, but unfortunately a little more common for RSIDs. _As mentioned above, a fix for this is being worked on._

### Visualization mode

![visualization_preview.gif](assets/visualization_preview.gif)

This is the mode that shows the animated spectrum analyser. _Plus a few other effects._ The controls in this mode are:

- Single-clicking returns to the song selection view.
- Double-clicking plays or pauses the selected song but keeps the visualization mode.
- Turning the encoder adjusts the volume. See below.

### Volume control

![volume.png](assets/volume.png)

This control is only available from the visualization mode and is activated by rotating the encoder. Turn the encoder
clockwise to increase the volume and counter-clockwise to decrease it. The current volume is displayed on screen as long
as the encoder is being turned. Plus 400ms after the last turn.

_Warning! With the current implementation the overall sound volume has increased since the first version. Be mindful of
this, if you're using headphones._

### Pocket mode

In this mode the screen is turned off, as well as the regular controls. Playback is still active though, which makes it
perfect for when you're out and about. To enter this mode, simply hold the button pressed until the screen goes black.
This takes exactly one second. Be careful not to hold it for too long, as this will put the SIDPod to sleep. More on
that in the next section. To get out of pocket mode, simply double-click, and it will return to whatever mode you were
in before entering pocket mode.

### Deep sleep

This mode is as close as you can get to turning the SIDPod off. As with pocket mode, all regular controls are disabled.
Additionally, playback is stopped, as well as USB device clocks. In fact, in this mode, even both processor cores are
stopped, meaning that the power consumption is next to nothing. To enter deep sleep, press the button for more than two
and a half second. To get out, hold the button until the SIDPod splash appears.

## Beta disclaimer

This is a beta version of the SIDPod. While it has been tested thoroughly, there are still some bugs and issues that
need to be worked out. And the entire code base would benefit from some cleanup and pattern alignment.

### General disclaimer

Even though I am a professional software engineer, I am a novice and a hobbyist when it comes to both hardware and C/C++
development. So there's a good chance that I have made more than a few mistakes. If you find any, please let me know, by
opening an issue on GitHub. I'll be happy to fix them and learn from them. Hopefully in a timely manner.

## What's on the horizon?

- Support for SD cards. This will allow you to store a lot more music than the internal flash can handle.
- Pi Pico 2 support. This will improve performance and playback quality. Potentially, this could allow for builtin
Bluetooth A2DP support. For Pi Pico 2W boards.
- "SIDPod Buddy" support. The SIDPod Buddy is an ESP32 based device that allows you to control the SIDPod using a
Cirque TM023023 Trackpad. Additionally, it provides Bluetooth A2DP support, which means that you can stream music to any
Bluetooth enabled speaker.
- A custom PCB, incorporating all the components needed to make a fully functional SIDPod. With SIDPod Buddy support.