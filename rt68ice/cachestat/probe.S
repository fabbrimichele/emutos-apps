/* GPL-2.0-or-later. Called only through the supervisor callback.
 * Restore the original stack directly on BERR, as in EmuTOS check_read_byte.
 * Interrupts are masked while the temporary vector is installed. */
        .text
        .even
        .globl _cachestat_probe
_cachestat_probe:
        move.w  %sr,-(%sp)
        ori.w   #0x0700,%sr
        move.l  %sp,%d1
        move.l  8,%a1
        lea     .Lprobe_error(%pc),%a0
        move.l  %a0,8
        moveq   #0,%d0
        nop
        move.w  0x00f30000,%d0
        nop
        bra     .Lprobe_done
.Lprobe_error:
        moveq   #-1,%d0
.Lprobe_done:
        move.l  %a1,8
        move.l  %d1,%sp
        move.w  (%sp)+,%sr
        rts
