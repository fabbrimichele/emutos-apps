# emutos-apps

Programs are compiled with [m68k-atari-mint](http://vincent.riviere.free.fr/soft/m68k-atari-mint/) that
uses MiNTLib producing really big executable, hello.tos is more than 130KB.

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
