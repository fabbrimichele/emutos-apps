/*
 * joytest.c - Display Atari IKBD joystick events.
 *
 * Compile with:
 *   make
 * Send with:
 *   make send-setting-port
 *
 * Distributed under the GPL, version 2 or later. See https://www.gnu.org/licenses/old-licenses/gpl-2.0.html.
 */

#include <stdio.h>
#include <osbind.h>

/* A word gives the foreground an atomic snapshot of both state bytes. */
volatile unsigned short joytest_states;
static _KBDVECS *vectors;
static void (*previous_joyvec)(void *);

void joytest_handler(void *packet);

/* IKBD event callbacks receive the packet in A0, not as a C argument.
 * Copy bytes individually: the source may not be word aligned.
 * Printing is done by the foreground, never by the interrupt callback.
 */
__asm__(
    ".text\n"
    ".globl _joytest_handler\n"
    "_joytest_handler:\n"
    "move.b 1(%a0),_joytest_states\n"
    "move.b 2(%a0),_joytest_states+1\n"
    "rts\n"
);

static long install_handler(void)
{
    previous_joyvec = vectors->joyvec;
    vectors->joyvec = joytest_handler;
    return 0;
}

static long restore_handler(void)
{
    vectors->joyvec = previous_joyvec;
    return 0;
}

/* VT52 cursor positioning keeps both rows visible on 40-column screens. */
static void show_state(unsigned int number, unsigned int state)
{
    char line[80];

    sprintf(line, "\033Y%c%c\033KJOY%u=%02X U:%u D:%u L:%u R:%u FIRE:%u\r\n",
            (int)(32 + 4 + number), 32, number, state,
            !!(state & 1), !!(state & 2), !!(state & 4),
            !!(state & 8), !!(state & 128));
    (void)Cconws(line);
}

int main(void)
{
    static const char enable[] = { 0x14 };
    static const char disable[] = { 0x1a };
    static const char relative_mouse[] = { 0x08 };
    unsigned short last = 0xffff;

    (void)Cconws("\033E\033fTOS joystick test\r\n"
           "Move a joystick and press fire.\r\n"
           "Any keyboard key exits.\r\n");
    vectors = Kbdvbase();
    Ikbdws(0, disable);
    Supexec(install_handler);
    Ikbdws(0, enable);

    while (!Cconis())
    {
        unsigned short current = joytest_states;

        if (current != last)
        {
            show_state(0, current >> 8);
            show_state(1, current & 255);
            last = current;
        }
    }

    (void)Cnecin();
    Ikbdws(0, disable);
    Supexec(restore_handler);
    Ikbdws(0, enable);
    /* Restore the usual mouse/joystick-1 configuration on Atari hardware. */
    Ikbdws(0, relative_mouse);
    (void)Cconws("\033e\033Y\047\040Finished.\r\n");
    return 0;
}
