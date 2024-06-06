# LittleGPTracker

LittleGPTracker (a.k.a 'The piggy') is a music tracker optimised to run on portable game consoles.

## Support matrix

attempt to define the features in each target supported by piggy

target      | platform | audio         | midi  | extra     | libc  | march
------------|----------|---------------|-------|-----------|-------|---------
BEAGLEBOARD | LINUX    | sdl           |       | soundfont | glibc | arm7h
BITTBOY     | DINGUX   | sdl           |       | soundfont | musl  |
CAANOO      |          |               |       | soundfont |       |
DEBIAN      | LINUX    |               |       | soundfont | glibc | x86
DINGOO      |          |               |       | soundfont |       | mips
GP2X        |          |               |       | soundfont |       | arm9tdmi
GP32        |          |               |       | soundfont |       | arm9tdmi
MIYOO       | DINGUX   | sdl           | dummy | soundfont | glibc | arm7h
NDS         |          |               |       | soundfont |       |
OSX         |          |               |       | soundfont |       | powerpc
PSP         | PSPSDK   | psp           |       | soundfont |       | mips
RASPI       |          |               |       | soundfont |       | arm7h
RS97        |          |               |       | soundfont |       | arm7h
STEAM       | LINUX    |               |       | soundfont | glibc | x86
WINDOWS     | WINDOWS  |               |       | soundfont |       | x86

* `target`: a configuration for a specific device or family of devices
* `platform`: OS for open platforms, SDK for homebrew ones
* `audio`: included audio drivers
* `midi`: included midi drivers
* `extra`: extra modules included
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
* Extra
    - `soundfont`: soundfont support, 32bit targets only

## Defines

defines are used to configure the build

define              | info
--------------------|------
`__LINUX_ALSA__`    | ??? use alsa directly
`__LINUX_ALSASEQ__` | ??? use alsa directly
`_64BIT`            | 64 bit compatiblity patches
`_DEBUG`            | include extra debug prints
`_NO_JACK_`         | exclude jack support from build
`_NO_RTAUDIO_`      | exclude rtaudio support from build
`_NO_RTMIDI_`       | exclude rtmidi support from build
`CPP_MEMORY`        | ???
