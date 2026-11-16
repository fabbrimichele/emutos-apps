# CACHSTAT.TOS — RT68ICE cache counters

Builds for a 68000 with libcmini. Requires the counter-enabled RT68ICE FPGA
bitstream (register ABI 1 at `$00F30000`), with no EmuTOS driver changes.
The utility probes the identity in supervisor mode with a temporary bus-error
handler, so a missing mapping is reported instead of using the OS panic path.
The probe was checked under headless Hatari/EmuTOS against missing hardware. The user has also validated a START/CoreMark/STOP measurement on the board.

```sh
make build check
make send-setting-port
```

Start a ZMODEM receiver such as `XYZ.TTP` on the target before the send command.
Default serial settings are 57600 8N1; `SZFLAGS=-b -e` enables binary transfer
with control-character escaping. The executable is `CACHSTAT.TOS`.

On EmuTOS, run from a shell with the utility and benchmark in the current
folder (or PATH):

```text
CACHSTAT START
COREMARK.TOS
CACHSTAT STOP
```

- `START`: clear all counters and enable counting.
- `STOP`: stop counting, take an atomic snapshot, print values and ratios.
- `SHOW` (or no arguments): stop/snapshot/print, like STOP.
- `CLEAR`: clear/stop and invalidate the old snapshot.
- `RESUME`: enable without clearing previous totals.
- `HELP`: show command usage without accessing hardware.

Counters start disabled after reset. Eight frozen 64-bit values report elapsed
system cycles, read hits/misses, writes, CPU read/write stalls and cache
read/write RAM waits. Clock frequency is read from the hardware. All numbers
are printed without relying on libcmini floating-point or `%llu` support.
The hit rate, elapsed seconds, average stall/wait cycles and stall shares are
calculated from that snapshot. Zero denominators print `n/a`.

The measurement includes program loading/exit and shell/desktop/interrupt work
between commands. START prints before enabling; STOP snapshots before printing.
SHOW also freezes; use RESUME to continue without clearing. Read stalls
include cache lookup; RAM waits are a subset, not additional stall cycles.
Reads combine instruction and data traffic. A four-word fill is one miss;
CPU longword accesses contain two 16-bit requests.

The hardware's full register ABI and event definitions are documented in
[RT68ICE cache statistics](https://github.com/fabbrimichele/rt68ice/blob/cache-performance-counters/doc/cache-stats.md).
The hardware counters, snapshots and bus decode have simulation coverage.
`make check` tests the C command/arithmetic paths using host mock MMIO; it does
not prove real-board behaviour. The actual missing-hardware probe was also
exercised under Hatari/EmuTOS.

Sources are GPL-2.0-or-later, consistent with the repository license.

A first on-board START/CoreMark/STOP run returned a valid frozen snapshot and
CoreMark validation at 12.654223 iterations/s. Its counter window was 59.886 s,
versus 15.805 s for CoreMark's timed region: this includes manual command entry
and surrounding OS activity, so it is not a CoreMark-only cache profile. Full
values and interpretation are recorded in the hardware documentation above.
