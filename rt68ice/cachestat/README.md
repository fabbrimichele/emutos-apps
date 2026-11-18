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
include cache lookup. Read RAM waits are a subset of CPU read stalls, not
additional stall cycles. With posted writes, the legacy write RAM-wait event
only covers the one-cycle cache write lookup when completion is not yet
captured; it does not measure background SDRAM write duration and is not a
subset of CPU write stalls. Use CPU write stalls to assess CPU blocking.
The tool still prints the ABI 1 counters as supplied by the hardware.
Reads combine instruction and data traffic. A four-word fill is one miss;
CPU longword accesses contain two 16-bit requests.

The hardware's full register ABI and event definitions are documented in
[RT68ICE cache statistics](https://github.com/fabbrimichele/rt68ice/blob/main/doc/cache-stats.md).
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


## Cache performance progression (2026-11-18)

These are user-supplied validated `COREMARK.TOS` measurements at 25 MHz
CPU/system and 50 MHz SDRAM, not new CACHSTAT profiling snapshots. Values are
arithmetic means of the supplied runs; the uncached run used 110 iterations,
later versions 200. The utility itself and the register ABI are unchanged.

| Hardware step | Mean CoreMark iterations/s | Change vs previous step | Change vs uncached |
| --- | ---: | ---: | ---: |
| No cache | 7.013069 | — | — |
| Original 2 KiB cache, one-word lines | 12.861736 | +83.40% | +83.40% |
| Registered request/response interface, 2 KiB | 11.012205 | -14.38% | +57.02% |
| 8 KiB cache, four-word burst fills | 12.581583 | +14.25% | +79.40% |
| Dedicated SDRAM input stage, invalidate on write hit | 12.578616 | -0.02% | +79.36% |
| Write-hit updates, explicit mux implementation | 14.324082 | +13.88% | +104.25% |
| Write-hit updates, Vec/byte-override refactor | 14.316391 | -0.05% | +104.14% |
| Single-entry posted-write buffer | 15.170670 | +5.97% | +116.32% |

The posted-write runs report 2637, 2636 and 2637 ticks (13.185, 13.180 and
13.185 seconds), all with correct-operation validation. YAART completed six
clean passes in 23:51 on a retry, now into pass seven. Its first four passes
took 15:54 rather than 17:16 without the buffer: 8.60% higher workload
throughput. An earlier run rebooted during pass five; the cause is unresolved.
Long breadboard reset wiring is a hypothesis, not a confirmed cause.

The write buffer acknowledges a CPU write after cache lookup/update while
SDRAM completes in the background. Cache read hits can proceed, while another
write or a read miss waits for the pending entry. It still uses single-word
SDRAM writes without write combining or bursts.

For exact per-run results, provenance, test coverage and outstanding checks,
see the [hardware validation history](https://github.com/fabbrimichele/rt68ice/blob/main/doc/validation.md#cache-performance-progression).
The posted-write sections become available on hardware main after its PR is
merged. No new on-board START/CoreMark/STOP snapshot has been supplied for this
version; the earlier 12.654223 result above remains a historical profiling run.
