/* POSIX file shim for exercising error handling, not target emulation.
 * SPDX-License-Identifier: MIT
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "osbind.h"
#include "time.h"
#include <stdint.h>
#include <stdio.h>

static int fault(const char *name)
{
    const char *s = getenv("FILETEST_FAULT");
    return s && strcmp(s, name) == 0;
}
static void trace(const char *operation, long count)
{
    const char *path = getenv("FILETEST_TRACE");
    if (path) {
        FILE *f = fopen(path, "a");
        if (f) { fprintf(f, "%s %ld\n", operation, count); fclose(f); }
    }
}

long Fopen(const char *p, int mode)
{
    int h = open(p, mode == 0 ? O_RDONLY : O_RDWR);
    return h < 0 ? (errno == ENOENT ? -33 : -36) : h;
}
long Fcreate(const char *p, int attr)
{
    int h;
    (void)attr;
    h = open(p, O_CREAT | O_TRUNC | O_RDWR, 0600);
    return h < 0 ? -36 : h;
}
long Fclose(int h)
{
    int result;
    trace("close", 0);
    result = close(h);
    return result || fault("close") ? -36 : 0;
}
long Fread(int h, long n, void *p)
{
    trace("read", n);
    if (fault("read")) return -11;
    if (fault("shortread") && n > 512) --n;
    return (long)read(h, p, (size_t)n);
}
long Fwrite(int h, long n, const void *p)
{
    trace(h == STDOUT_FILENO ? "output" : "write", n);
    if (h == STDOUT_FILENO) return (long)write(h, p, (size_t)n);
    if (fault("corruptpayload") && n == 16384) {
        unsigned char first = ((const unsigned char *)p)[0] ^ 0x80;
        long a = (long)write(h, &first, 1);
        long b = (long)write(h, (const unsigned char *)p + 1, (size_t)n - 1);
        return a < 0 || b < 0 ? -10 : a + b;
    }
    if (fault("write")) return -10;
    if (fault("shortwrite") && n > 1) --n;
    return (long)write(h, p, (size_t)n);
}
long Cconis(void) { return fault("abort") ? -1 : 0; }
long Cnecin(void) { return 27; }

/* Deterministic timing for rate-format and 32-bit wrap tests. */
clock_t clock(void)
{
    static uint32_t tick;
    static int initialized;
    const char *step = getenv("FILETEST_CLOCK_STEP");
    if (!initialized) {
        const char *start = getenv("FILETEST_CLOCK_START");
        tick = start ? (uint32_t)strtoul(start, 0, 0) : 0;
        initialized = 1;
    }
    trace("clock", 0);
    tick += step ? (uint32_t)strtoul(step, 0, 0) : 400;
    return (clock_t)tick;
}
