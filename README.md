# LittleGPTracker (aka piggy, piggytracker)

ridiculously versatile and portable LSDJ alike music tracker

## Build

compile the software from the projects directory, requires GCC, gnu make, various toolchains

```
# build for desktop linux on x86_64
make PLATFORM=X64

# build for desktop linux 32 bit
# (theoretically equivalent to old DEB target)
make PLATFORM=X86

# build for RG35XX+ family devices
make PLATFORM=PLUSH
```

## Config

there are no default keybinds use a `config.xml` file

apologies for vim binds

```
<CONFIG>
  <!-- paths -->
  <ROOTFOLDER value="/path/to/tracks"/>
  <SAMPLELIB value="/path/to/samples"/>
  <!-- appearance -->
  <!--
  <SCREENMULT value="1"/>
  <BACKGROUND value="0F0A1C"/>
  <FOREGROUND value="EB0350"/>
  <HICOLOR1 value="FFFFFF"/>
  <HICOLOR2 value="0AA290"/>
  <ROWCOLOR1 value="AAAAAA"/>
  <ROWCOLOR2 value="FFFFFF"/>
  <SONGVIEW_00 value="5D021E"/>
  <SONGVIEW_FE value="5D021E"/>
  -->
  <!-- keybinds -->
  <KEY_UP value="k" />
  <KEY_DOWN value="j" />
  <KEY_LEFT value="h" />
  <KEY_RIGHT value="l" />
  <KEY_A value="x" />
  <KEY_B value="z" />
  <KEY_LSHOULDER value="left shift" />
  <KEY_RSHOULDER value="space" />
  <KEY_START value="return" />
  <!--
  <DUMPEVENT value="YES"/><!-- uncomment to log keypresses -->
  <RENDER value="FILE"/><!-- uncomment to render -->
  -->
</CONFIG>
```

## Support matrix

attempt to define the features in each target supported by piggy

platform    | family  | audio         | midi   | libc  | march
------------|---------|---------------|--------|-------|---------
BEAGLEBOARD | LINUX   | sdl           |        | glibc | arm7h
BITTBOY     | DINGUX  | sdl           |        | musl  |
CAANOO      | CAANOO  |               |        |       |
DEBIAN      | LINUX   |               |        | glibc | x86
DINGOO      | DINGUX  |               |        |       | mips
GP2X        |         |               |        |       | arm9tdmi
GP32        |         |               |        |       | arm9tdmi
MIYOO       | DINGUX  | sdl           | dummy  | glibc | arm7h
NDS         | NDS     | nds           |        |       |
OSX         |         |               |        |       | powerpc
PSP         | PSPSDK  | psp           |        |       | mips
RASPI       |         |               |        |       | arm7h
PLUSH       | DINGUX  | sdl           | dummy  | glibc | arm7h
RS97        |         |               |        |       | arm7h
STEAM       | LINUX   |               |        | glibc | x86
WINDOWS     | WINDOWS |               |        |       | x86
X64         | LINUX   | sdl           | rtmidi | glibc | x86_64
X86         | LINUX   | rtaudio, jack | rtmidi | glibc | x86

* `platform`: a configuration for a specific device or family of devices
* `family`: OS for open platforms, SDK for homebrew ones
* `audio`: configured audio driver
* `midi`: configured midi driver
* `libc`: which libc is being linked against
* `march`: microarchitecture being built for

## Modules

modules are features that can be optionally included in the build

* Audio
    - `OSS` alsa precursor
    - `RTAudio` cross platform audio lib
    - `SDLAudio` sdl's inbuilt audio output
    - `JACK` low latency pro audio for linux
    - `dummy` fake audio driver for debugging
* Midi
    - `RTMidi` cross platform audio lib
    - `BITTBOYSerialMidi` custom serial midi for bittboy handhelds
    - `JACK` jack's midi support
    - `dummy` fake midi driver for platforms without IO

## Defines

defines are used to configure the build

define              | info
--------------------|---------------------------------------------
`__LINUX_ALSA__`    | ??? use alsa directly
`__LINUX_ALSASEQ__` | ??? use alsa directly
`_64BIT`            | 64 bit compatibility patches
`_DEBUG`            | include extra debug prints
`BUFFERED`          | use page flipping instead of region blitting
`CPP_MEMORY`        | ???
`DUMMYMIDI`         | use dummy midi driver
`JACKAUDIO`         | use Jack audio output
`JACKMIDI`          | use Jack midi driver
`RTAUDIO`           | use RtAudio output
`RTMIDI`            | use RtMidi driver
`SDLAUDIO`          | use SDL audio output

legacy defines, to be removed

define              | info
--------------------|---------------------------------------------
`_NO_JACK_`         | exclude jack support from build
`_NO_RTAUDIO_`      | exclude rtaudio support from build
`_NO_RTMIDI_`       | exclude rtmidi support from build
