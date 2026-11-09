# Generic TOS/EmuTOS file integrity and speed test

`FILETEST.TTP` tests file integrity and sequential file-I/O speed using
standard GEMDOS file calls. It needs a 68000 or later and no GEM, MiNT, FPU or Atari-specific
registers. It belongs alongside the generic apps, not under `rt68ice/`.

## Build

```sh
make
make check
```

Uses `m68k-atari-mint-gcc` and libcmini, like the other apps. Override
`LIBCMINI` with the directory containing `crt0.o` if necessary. `make check`
runs the real application code with a POSIX GEMDOS shim: it checks file
lifecycle, corruption/truncation detection, overwrite refusal and injected
I/O failures, CRLF output and elapsed-time/rate formatting (including timer
wrap), both file formats and sequential transfer boundaries. The host clock
is simulated; these checks do not validate a board,
SD card or target runtime.

## Repeated write/read verification

Copy `filetest.ttp` to the target. From EmuCON, for example:

```text
FILETEST.TTP run C:\SDTEST.DAT 4 10
```

This performs ten passes, each writing a 4 MiB payload, closing the file,
reopening it, comparing every byte, checking EOF and closing it again.
The seed changes every pass. On success it prints `SUCCESS` and retains the
last file. Exit status is zero on success and one on error, abort or invalid
arguments. Hold ESC to abort between transfers; a partial file may remain.

`run`, `speed` and `write` refuse an existing pathname at startup. Choose a new name
or explicitly remove the previous test file before starting another run.
Within a run, subsequent passes intentionally replace that run's test file.
No raw sectors, partitions or unrelated files are written. Choose a volume
with enough free space; a full disk is reported as a write failure.

## Sequential speed test

```text
FILETEST.TTP speed C:\SPEED.DAT 4 3
```

This performs three passes using a 4 MiB payload. Choose a new filename.
Each pass measures sequential WRITE and READ in fixed **16 KiB transfers**,
then reopens the file again to verify every byte **outside the timed phases**.
It retains the last file and prints `SUCCESS` only if all phases and the
untimed verification succeed. To start with one pass, use `4 1` instead.

The WRITE and READ summaries report seconds and KiB/s, labelled
`sequential file I/O`. In contrast, `run`/`write`/ordinary `verify` timings
remain labelled `end-to-end`. No measured results are built into the program.

Timing boundaries:

- The write buffer is generated before the WRITE timer starts. Its first
  four bytes are tagged with the block index before each transfer, so the
  later verifier can detect swapped or repeated blocks. Only this small
  tag update happens inside the timed write loop.
- File creation, opening and header transfers are excluded from both timers.
  The 512-byte speed header aligns payload transfers to 512-byte file offsets.
- WRITE includes all payload `Fwrite` calls and the final `Fclose`, including
  filesystem flushing performed by that call.
- READ includes all payload `Fread` calls, the EOF check and the final
  `Fclose`. No byte comparison or checksum runs in this timed loop.
- There is no progress output inside either timed loop. ESC is polled once
  per block. The existing 200 Hz timer and integer rate calculations are used.
- After both timed phases, verification rereads and checks the complete file.
  The saved phase durations are printed only after this succeeds. Its extra
  read and comparison time is not included in the reported speeds.

These measure **sequential file I/O through GEMDOS**, including driver, SPI,
filesystem and small loop/abort-poll overhead. They are not raw SPI bit rates.
A read immediately after writing may benefit from OS/card caches; this test
does not force uncached access or power-loss durability. Compare runs on the
same filesystem and distinguish these rates from the mixed-size integrity
workload, which also generates and checks data inside its measured phases.

Speed files have a `FILESPD1` header containing the size, seed and pass, padded
with zeros to 512 bytes. Their payload reuses a 16 KiB pattern with a different
block-index tag in each block. The mixed-size integrity test uses the original
`FILETST1` header and a continuous pattern; it remains the broader integrity
workload. The updated `verify` command recognizes **both formats**, including
files created by the earlier program:

```text
FILETEST.TTP verify C:\SPEED.DAT
```

This also allows normal reboot followed by verification of a completed speed
file. Earlier executables cannot verify the new speed-file format.

## Reboot persistence check

```text
FILETEST.TTP write C:\SDTEST.DAT 4 0x12345678
```

Wait for `WRITE COMPLETE` (the close has succeeded), reboot normally, then:

```text
FILETEST.TTP verify C:\SDTEST.DAT
```

The 32-byte header stores the payload length, seed and pass. `verify` needs
no seed or size argument and opens the file read-only. All modes work from
the Desktop's `.TTP` argument dialog as well as a shell; enter the arguments
without `FILETEST.TTP` in that dialog. No arguments prints usage.

## Parameters and coverage

```text
FILETEST.TTP run FILE [MiB [PASSES [SEED]]]
FILETEST.TTP speed FILE [MiB [PASSES [SEED]]]
FILETEST.TTP write FILE [MiB [SEED]]
FILETEST.TTP verify FILE
```

Defaults: 4 MiB, ten integrity passes (`run`), three speed passes (`speed`),
seed `0x12345678`. Limits: 1..64 MiB and 1..10000 passes. Seeds are unsigned 32-bit decimal or `0x` hexadecimal.
The file occupies the payload size plus 32 bytes (`run`/`write`) or 512 bytes
(`speed`); only about 16 KiB of
payload buffer memory is needed, not a buffer the size of the whole file.

For the mixed-size integrity workload (`run`/`write`):

- Transfer lengths cycle through 1, 511, 512, 513, 4095, 4096, 4097 and
  16384 bytes. The reader starts at different boundaries from the writer.
- A streaming xorshift32 pattern continues across transfers. Each generated
  32-bit word is emitted most-significant byte first. Zero seeds are usable.
- The header has an FNV-1a integrity check; all payload bytes are compared
  against independently regenerated expected values. Run mode also checks
  that the reopened header matches the current pass.
- Errors report GEMDOS return values or the pass, absolute file offset,
  expected byte and actual byte. Offset zero is the start of the header.
- Progress is printed roughly every MiB. Output uses explicit CRLF so each
  new line starts at the left margin on TOS and serial terminals.
- After a successful phase, elapsed time and effective payload KiB/s are
  printed separately for WRITE and VERIFY. Both include opening/creating,
  header I/O, payload processing and closing the file. VERIFY also includes
  the EOF check. Timing ends before the timing/result summary is printed.
- Timing uses libcmini's `clock()`, which reads the TOS 200 Hz system tick
  (5 ms resolution). Unsigned subtraction handles a single tick-counter
  wrap. Zero elapsed ticks report an unavailable rate; calculations use
  integer arithmetic, with three decimal places for seconds and one for
  KiB/s. The file header is excluded from the rate's byte count.
- These are **end-to-end integrity-test rates**, including pattern generation,
  byte comparisons and progress output. They are not raw SD throughput
  figures and may be substantially slower than a sequential transfer test.

A pass tests ordinary file I/O through the OS and its underlying driver.
Immediate readback alone does not prove power-loss durability or that every
read bypasses filesystem caches. The reboot/verify mode adds a persistence
check. This also does not test all card sectors, filesystem recovery from
interrupted writes, or hidden flash capacity. No particular hardware has
been validated merely by building this program.

## Transfer by serial

```sh
make send-setting-port SERIAL_PORT=/dev/ttyACM0 SERIAL_BAUD=57600
```

Close any host terminal using the serial port. Start XYZ on the target in
ZMODEM receive mode (XYZ 2.02B defaults to receiving when invoked without
arguments), then promptly run the command above. Restart XYZ before retrying
a transfer if its receive timeout has expired. Remove any old executable
before receiving it.

The serial target explicitly sets raw binary mode, 57600 baud by default,
8N1, no echo and no RTS/CTS or XON/XOFF flow control. The sender uses binary
mode and escapes control characters (`sz -b -e`). `make send` uses the
port settings already configured, and `SZFLAGS` overrides sender options.

If both sides time out, cancel the sender with Ctrl-C and restart the
receiver. Confirm that `SERIAL_PORT` names the board and no other process
is reading it (for example `fuser /dev/ttyACM0`). Inspect the active settings
with `stty -F /dev/ttyACM0 -a`. A timeout alone does not identify whether
the cause is port settings, receiver state or target serial reception.

License: MIT, as in the repository root.
