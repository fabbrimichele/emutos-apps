# TOS joystick test

A console application for TOS and EmuTOS systems with IKBD-compatible
joystick support. It uses `Kbdvbase()`, `Ikbdws()` and the standard `joyvec`
callback, without accessing machine-specific hardware registers.

Build with `make`. This creates `joytest.prg` using libcmini and targets the
68000 instruction set. Copy the executable to the target computer and run
it. For Hatari, `make deploy` copies it to the configured applications folder.

The display uses TOS VT52 cursor positioning and CRLF line endings. Both
joysticks have separate fixed rows that fit a 40-column screen.

Move each direction and press fire. Directions are up `01`, down `02`,
left `04`, and right `08`; fire is `80`. Combinations OR the bits together.
Release everything and check that the state returns to `00`. With two
joysticks, holding a control on one while moving the other should preserve
both states. On Atari hardware, joystick 0 shares the mouse port: connect a
joystick there to test it.

Press any keyboard key to exit. The program restores the previous joystick
callback and the usual joystick event/relative mouse reporting configuration.
It tests event callbacks; interrogation mode is not tested.

## RT68ICE connections and serial transfer

On RT68ICE, USB port 3 is joystick 1 and USB port 4 is joystick 0. Keyboard
and mouse use ports 1 and 2. A/B/X/Y each act as fire; Start and Select are
ignored by the RT68ICE driver.

Start a ZMODEM receiver on the target (for example XYZ.TTP), delete any old
copy of `joytest.prg` there, then run:

```sh
make send-setting-port SERIAL_PORT=/dev/ttyACM0 SERIAL_BAUD=57600
```

`make send` uses the serial settings already configured on the host.
