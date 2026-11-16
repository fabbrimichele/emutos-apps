/* RT68ICE cache counters, register ABI 1. GPL-2.0-or-later. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <osbind.h>

#define BASE 0x00F30000UL
#define RUN 1
#define CLEAR 2
#define SNAPSHOT 4

enum action { SHOW, START, STOP, RESET, RESUME };
static enum action action;
static uint64_t values[8];
static unsigned long clock_hz;
static unsigned int running, valid;

extern long cachestat_probe(void);
#ifdef CACHESTAT_HOST_TEST
extern unsigned int cachestat_read16(unsigned int offset);
extern void cachestat_write16(unsigned int offset, unsigned int value);
#else
static unsigned int cachestat_read16(unsigned int offset)
{
    return *(volatile unsigned short *)(BASE + offset);
}
static void cachestat_write16(unsigned int offset, unsigned int value)
{
    *(volatile unsigned short *)(BASE + offset) = (unsigned short)value;
}
#endif

/* All MMIO, including the guarded identity probe, is done in supervisor mode.
 * Printing happens after leaving this callback, from a consistent frozen bank.
 */
static long access_counters(void)
{
    unsigned int command, i, word;
    if (cachestat_probe() != 0x4343L)
        return -1;
    if (cachestat_read16(2) != 1)
        return -2;
    switch (action) {
    case START: command = RUN | CLEAR; break;
    case STOP: command = SNAPSHOT; break;
    case RESET: command = CLEAR; break;
    case RESUME: command = RUN; break;
    default: command = SNAPSHOT; break;
    }
    cachestat_write16(4, command);
    if (action == START || action == RESUME || action == RESET)
        return 0;
    running = cachestat_read16(4) & RUN;
    valid = cachestat_read16(6) & 1;
    clock_hz = ((unsigned long)cachestat_read16(12) << 16) | cachestat_read16(14);
    for (i = 0; i < 8; ++i) {
        values[i] = 0;
        for (word = 0; word < 4; ++word)
            values[i] = (values[i] << 16) | cachestat_read16(0x20 + i * 8 + word * 2);
    }
    return 0;
}

/* libcmini's printf need not provide long-long or floating-point formatting. */
static void decimal(uint64_t n, char *out)
{
    char reverse[21];
    unsigned int i = 0, j = 0;
    do {
        reverse[i++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (i) out[j++] = reverse[--i];
    out[j] = 0;
}

/* One decimal long-division digit, without overflowing remainder * 10. */
static unsigned int digit(uint64_t *remainder, uint64_t divisor)
{
    uint64_t add = *remainder, sum = 0;
    unsigned int i, result = 0;
    for (i = 0; i < 10; ++i) {
        if (sum >= divisor - add) {
            sum -= divisor - add;
            ++result;
        } else {
            sum += add;
        }
    }
    *remainder = sum;
    return result;
}

static void ratio(const char *label, uint64_t numerator, uint64_t denominator, int percent)
{
    char whole[21];
    unsigned int digits[4], i;
    uint64_t q, r;
    if (!denominator) {
        printf("%-23s n/a\r\n", label);
        return;
    }
    q = numerator / denominator;
    r = numerator % denominator;
    for (i = 0; i < 4; ++i) digits[i] = digit(&r, denominator);
    if (percent) {
        /* Percentage callers always have numerator <= denominator. */
        decimal(q * 100 + digits[0] * 10 + digits[1], whole);
        printf("%-23s %s.%u%u%%\r\n", label, whole, digits[2], digits[3]);
    } else {
        decimal(q, whole);
        printf("%-23s %s.%u%u%u\r\n", label, whole, digits[0], digits[1], digits[2]);
    }
}

static void show(void)
{
    static const char *labels[8] = {
        "Elapsed system cycles", "Read hits", "Read misses", "Write requests",
        "Read stall cycles", "Write stall cycles", "Read RAM-wait cycles", "Write RAM-wait cycles"
    };
    unsigned int i;
    char number[21];
    uint64_t reads = values[1] + values[2];
    printf("RT68ICE cache counters (ABI 1)\r\n");
    printf("Counting: %s; snapshot: %s\r\n", running ? "running" : "stopped", valid ? "valid" : "invalid");
    printf("Clock: %lu Hz\r\n", clock_hz);
    if (!valid) return;
    for (i = 0; i < 8; ++i) {
        decimal(values[i], number);
        printf("%-23s %s\r\n", labels[i], number);
    }
    ratio("Elapsed seconds", values[0], clock_hz, 0);
    ratio("Read hit rate", values[1], reads, 1);
    ratio("Read stall / read", values[4], reads, 0);
    ratio("Write stall / write", values[5], values[3], 0);
    ratio("RAM wait / read miss", values[6], values[2], 0);
    ratio("RAM wait / write", values[7], values[3], 0);
    ratio("Read stall share", values[4], values[0], 1);
    ratio("Write stall share", values[5], values[0], 1);
    if (running)
        printf("Counters must be stopped before reading.\r\n");
}

static int equals(const char *a, const char *b)
{
    while (*a && *b) {
        char c = *a++;
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        if (c != *b++) return 0;
    }
    return !*a && !*b;
}

int main(int argc, char **argv)
{
    long result;
    const char *name = argc == 1 ? "SHOW" : argv[1];
    if (argc > 2 || equals(name, "HELP") || equals(name, "--HELP") || equals(name, "-H")) {
        printf("CACHSTAT [START|STOP|SHOW|CLEAR|RESUME]\r\n"
               "START clears/enables; STOP freezes/prints; SHOW also freezes/prints.\r\n"
               "CLEAR clears/disables; RESUME enables without clearing.\r\n"
               "Requires RT68ICE cache counter hardware; default is SHOW.\r\n");
        return argc > 2 ? 1 : 0;
    }
    if (equals(name, "START")) action = START;
    else if (equals(name, "STOP")) action = STOP;
    else if (equals(name, "SHOW")) action = SHOW;
    else if (equals(name, "CLEAR")) action = RESET;
    else if (equals(name, "RESUME")) action = RESUME;
    else {
        printf("Unknown command: %s. Use CACHSTAT HELP.\r\n", name);
        return 1;
    }
    /* Print before enabling so the START/RESUME message does not enter the window. */
    if (action == START || action == RESUME)
        printf("%s cache counters.\r\n", action == START ? "Starting" : "Resuming");
    result = Supexec(access_counters);
    if (result) {
        printf("%s\r\n", result == -1 ? "Cache counter hardware not found. Load the counter-enabled FPGA bitstream."
                                     : "Unsupported cache counter register version.");
        return 1;
    }
    if (action == SHOW || action == STOP) show();
    else if (action == RESET) printf("Cache counters cleared and stopped.\r\n");
    return 0;
}
