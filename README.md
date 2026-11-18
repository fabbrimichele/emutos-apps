# emutos-apps

Programs are compiled with [m68k-atari-mint](http://vincent.riviere.free.fr/soft/m68k-atari-mint/) that
uses MiNTLib producing really big executable, hello.tos is more than 130KB.

## File integrity testing

[filetest](filetest/README.md) is a generic TOS/EmuTOS console test that writes,
closes, reopens and verifies files through GEMDOS. It also supports verification
after reboot and does not require RT68ICE-specific hardware.

## RT68ICE cache profiling

[CACHSTAT.TOS](rt68ice/cachestat/README.md) controls the FPGA cache counters
from EmuTOS: clear/start, stop/snapshot, show and resume. It requires the
counter-enabled bitstream and prints 64-bit event and stall-cycle statistics.
Its README records the [cache performance progression](rt68ice/cachestat/README.md#cache-performance-progression-2026-11-18),
including the single-entry posted-write buffer's 5.97% CoreMark gain and the
legacy write RAM-wait counter limitation.

## libcmini
For small footprint install [libcmini](https://github.com/freemint/libcmini):
* download latest release, e.g. [libcmini-0.54.tar.gz](https://github.com/freemint/libcmini/releases/download/v0.54/libcmini-0.54.tar.gz)
* unzip it into `/usr`
* create a simlink to `libcmini`
* create the file `/etc/profile.d/libcmini.sh` with the content `export LIBCMINI=/usr/libcmini`
  
## libraries location
* /usr/m68k-atari-mint
* /usr/libcmini

## Resources
* [libcmini, a small-footprint C library](https://github.com/freemint/libcmini)
* [GEMlib doc](http://arnaud.bercegeay.free.fr/gemlib/html/)
* [GEMlib homepage](http://arnaud.bercegeay.free.fr/gemlib/)
* [GEMlib src](https://github.com/freemint/gemlib)
* [Code for using the Atari hardware using all possible systems functions](https://github.com/pmandin/cleancode)
* https://github.com/ggnkua/Atari_ST_Sources
* [Read Joystick, apparently you need your own subroutine](https://github.com/simonsunnyboy/stlibs4gcc/tree/master/ikbd)
* [Reading joystick tutorial](https://www.atarimagazines.com/v5n1/joystickst.html)
