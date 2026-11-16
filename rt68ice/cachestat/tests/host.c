#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

static uint64_t live[8] = {50000000, 300, 100, 50, 1000, 500, 600, 450};
static uint64_t snapshot[8];
static unsigned int running, valid;
long Supexec(long (*function)(void)) { return function(); }
long cachestat_probe(void) { return getenv("MISSING") ? -1 : 0x4343; }
unsigned int cachestat_read16(unsigned int offset)
{
    if (offset == 2) return getenv("BAD_VERSION") ? 2 : 1;
    if (offset == 4) return running;
    if (offset == 6) return valid;
    if (offset == 12) return 25000000UL >> 16;
    if (offset == 14) return 25000000UL & 0xFFFF;
    assert(offset >= 0x20 && offset <= 0x5E && !(offset & 1));
    return (snapshot[(offset - 0x20) / 8] >> ((3 - ((offset - 0x20) % 8) / 2) * 16)) & 0xFFFF;
}
void cachestat_write16(unsigned int offset, unsigned int value)
{
    unsigned int i;
    const char *expected = getenv("EXPECT_COMMAND");
    assert(offset == 4);
    if (getenv("RUNNING")) running = 1;
    if (getenv("LARGE")) live[0] = UINT64_C(0x1234567900000000);
    if (getenv("ZERO")) for (i = 0; i < 8; ++i) live[i] = 0;
    if (expected) assert(value == (unsigned int)atoi(expected));
    running = value & 1;
    if (value & 2) {
        for (i = 0; i < 8; ++i) live[i] = snapshot[i] = 0;
        valid = 0;
    } else if (value & 4) {
        running = 0;
        for (i = 0; i < 8; ++i) snapshot[i] = live[i];
        valid = 1;
    }
}
/* Set initial RUN before SHOW reads control. */
__attribute__((constructor)) static void initial(void) { running = getenv("RUNNING") != 0; }
