# Palette

`palette.prg` is a GEM/VDI test application for the RT68ICE video modes.
It fills the current screen with a 16 by 16 grid, displaying palette indices
0 through 255.  It reads the active resolution from VDI, so the grid uses the
whole screen in each selected mode, including 320x240, 640x240, and 640x480.

The program uses VDI drawing calls, not direct framebuffer writes.  This is
important because RT68ICE's 8-plane screen memory is planar rather than packed
one-byte-per-pixel memory.

## Controls

- Press any key to exit.
- Click the left mouse button to exit.

## Build

From the `emutos-apps` directory:

```sh
make -C rt68ice/palette
```

The resulting Atari executable is `rt68ice/palette/palette.prg`.

## Transfer to RT68ICE

Start a ZMODEM receiver on the RT68ICE, such as `XYZ.TTP`, then run:

```sh
make -C rt68ice/palette send
```

To configure the local serial port to 57600 baud, 8 data bits, no parity, and
one stop bit before sending:

```sh
make -C rt68ice/palette send-setting-port
```

The default serial device is `/dev/ttyACM0`.  Override it when needed:

```sh
make -C rt68ice/palette send SERIAL_PORT=/dev/ttyUSB0
```

Delete the old `palette.prg` on the RT68ICE before receiving a replacement.

## Deploy to Hatari

```sh
make -C rt68ice/palette deploy
```

This copies the executable to `/home/michele/hatari/Harddisk/APPS/` by default.
Use `DEPLOY_PATH=/your/path` to select a different destination.
