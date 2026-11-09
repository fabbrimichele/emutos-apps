/* Generic GEMDOS file integrity test. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <osbind.h>

#define HEADER_SIZE 32U
#define SPEED_HEADER_SIZE 512U
#define BUFFER_SIZE 16384U
#define MAX_MIB 64UL
#define DEFAULT_SEED 0x12345678UL

static unsigned char buffer[BUFFER_SIZE];
static const unsigned char magic[8] = {'F','I','L','E','T','S','T','1'};
static const unsigned char speed_magic[8] = {'F','I','L','E','S','P','D','1'};
static const unsigned int chunks[] = {1, 511, 512, 513, 4095, 4096, 4097, 16384};

/* Use one output path with explicit CRLF. GCC can turn constant printf
 * calls into puts(), whose newline handling differs in older libcmini.
 * GEMDOS stdout also preserves command-line redirection.
 */
static void print(const char *format, ...)
{
    char text[1024];
    va_list args;
    int count;
    va_start(args, format);
    count = vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (count <= 0) return;
    if ((unsigned int)count >= sizeof(text)) count = (int)sizeof(text) - 1;
    (void)Fwrite(1, (long)count, text);
}

/* libcmini clock() reads the TOS 200 Hz tick via Supexec. Subtract unsigned
 * values to handle a counter wrap without signed-overflow arithmetic.
 */
static void print_timing(const char *phase, uint32_t bytes, uint32_t ticks,
                         const char *scope)
{
    unsigned long seconds = (unsigned long)(ticks / CLOCKS_PER_SEC);
    unsigned long millis = (unsigned long)(((uint64_t)(ticks % CLOCKS_PER_SEC)
                                           * 1000) / CLOCKS_PER_SEC);
    if (ticks == 0) {
        print("  %s time: <1 timer tick; rate unavailable.\r\n", phase);
    } else {
        uint32_t rate10 = (uint32_t)(((uint64_t)bytes * CLOCKS_PER_SEC * 10)
                                    / ((uint64_t)ticks * 1024));
        print("  %s time: %lu.%03lu s; %lu.%lu KiB/s (%s).\r\n",
              phase, seconds, millis, (unsigned long)(rate10 / 10),
              (unsigned long)(rate10 % 10), scope);
    }
}

static void show_timing(const char *phase, uint32_t bytes, uint32_t started)
{
    print_timing(phase, bytes, (uint32_t)clock() - started, "end-to-end");
}

struct pattern {
    uint32_t state, word;
    unsigned int remaining;
};

static void pattern_init(struct pattern *p, uint32_t seed)
{
    /* Add a constant so seed zero is usable; xorshift must not start at zero. */
    p->state = seed + UINT32_C(0x9e3779b9);
    if (!p->state) p->state = UINT32_C(0x6d2b79f5);
    p->word = 0;
    p->remaining = 0;
}

static unsigned char pattern_byte(struct pattern *p)
{
    unsigned char value;
    if (!p->remaining) {
        p->state ^= p->state << 13;
        p->state ^= p->state >> 17;
        p->state ^= p->state << 5;
        p->word = p->state;
        p->remaining = 4;
    }
    value = (unsigned char)(p->word >> 24);
    p->word <<= 8;
    --p->remaining;
    return value;
}

static void put32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static uint32_t get32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t header_hash(const unsigned char *p)
{
    uint32_t h = UINT32_C(2166136261);
    unsigned int i;
    for (i = 0; i < 28; ++i) h = (h ^ p[i]) * UINT32_C(16777619);
    return h;
}

static void make_header(unsigned char *h, uint32_t size, uint32_t seed,
                        uint32_t pass)
{
    memcpy(h, magic, sizeof(magic));
    put32(h + 8, 1);               /* format version */
    put32(h + 12, size);           /* payload length, excludes header */
    put32(h + 16, seed);
    put32(h + 20, pass);
    put32(h + 24, HEADER_SIZE);
    put32(h + 28, header_hash(h));
}

static int cancelled(void)
{
    if (Cconis()) {
        long key = Cnecin();
        if ((key & 255) == 27) {
            print("ABORTED: ESC pressed; test file retained.\r\n");
            return 1;
        }
    }
    return 0;
}

static int close_file(int handle)
{
    long result = Fclose(handle);
    if (result != 0) {
        print("FAIL: Fclose returned %ld.\r\n", result);
        return 0;
    }
    return 1;
}

static int transfer_ok(const char *operation, long actual, long wanted,
                       uint32_t offset)
{
    if (actual == wanted) return 1;
    print("FAIL: %s at file offset %lu: requested %ld, returned %ld.\r\n",
           operation, (unsigned long)offset, wanted, actual);
    return 0;
}

static int new_path(const char *path)
{
    long h = Fopen(path, 0);
    if (h >= 0) {
        print("REFUSED: %s already exists. Choose a new name or remove it yourself.\r\n", path);
        (void)close_file((int)h);
        return 0;
    }
    if (h != -33) { /* EFILNF; other errors are not permission to create. */
        print("FAIL: cannot check %s: Fopen returned %ld.\r\n", path, h);
        return 0;
    }
    return 1;
}

static int write_file(const char *path, uint32_t size, uint32_t seed,
                      uint32_t pass)
{
    unsigned char header[HEADER_SIZE];
    struct pattern p;
    uint32_t started;
    uint32_t offset = 0, next_progress = UINT32_C(1048576);
    unsigned int step = (unsigned int)(pass % 8);
    long handle, result;
    int ok = 0;

    print("Pass %lu WRITE: %lu payload bytes, seed 0x%08lX\r\n",
           (unsigned long)pass, (unsigned long)size, (unsigned long)seed);
    started = (uint32_t)clock();
    handle = Fcreate(path, 0);
    if (handle < 0) {
        print("FAIL: Fcreate returned %ld.\r\n", handle);
        return 0;
    }
    make_header(header, size, seed, pass);
    result = Fwrite((int)handle, (long)HEADER_SIZE, header);
    if (!transfer_ok("Fwrite header", result, HEADER_SIZE, 0)) goto done;
    pattern_init(&p, seed);
    while (offset < size) {
        unsigned int i, count = chunks[step++ % 8];
        if ((uint32_t)count > size - offset) count = (unsigned int)(size - offset);
        if (cancelled()) goto done;
        for (i = 0; i < count; ++i) buffer[i] = pattern_byte(&p);
        result = Fwrite((int)handle, (long)count, buffer);
        if (!transfer_ok("Fwrite", result, count, HEADER_SIZE + offset)) goto done;
        offset += count;
        if (offset >= next_progress) {
            print("  Written %lu bytes\r\n", (unsigned long)offset);
            next_progress += UINT32_C(1048576);
        }
    }
    ok = 1;
done:
    if (!close_file((int)handle)) ok = 0;
    if (ok) show_timing("WRITE", size, started);
    return ok;
}

/* In run mode, also compare the header against what was just written. */
static int verify_file(const char *path, int known, uint32_t expected_size,
                       uint32_t expected_seed, uint32_t expected_pass)
{
    unsigned char header[HEADER_SIZE];
    struct pattern p;
    uint32_t started = (uint32_t)clock();
    uint32_t size, seed, pass, header_size, offset = 0, next_progress = UINT32_C(1048576);
    unsigned int step;
    long handle, result;
    int ok = 0, speed_format;

    handle = Fopen(path, 0);
    if (handle < 0) {
        print("FAIL: Fopen for verification returned %ld.\r\n", handle);
        return 0;
    }
    result = Fread((int)handle, (long)HEADER_SIZE, header);
    if (!transfer_ok("Fread header", result, HEADER_SIZE, 0)) goto done;
    size = get32(header + 12);
    seed = get32(header + 16);
    pass = get32(header + 20);
    speed_format = memcmp(header, speed_magic, sizeof(speed_magic)) == 0;
    header_size = speed_format ? SPEED_HEADER_SIZE : HEADER_SIZE;
    if ((!speed_format && memcmp(header, magic, sizeof(magic)))
        || get32(header + 8) != 1
        || get32(header + 24) != header_size
        || get32(header + 28) != header_hash(header)
        || size == 0 || size > MAX_MIB * UINT32_C(1048576) || pass == 0) {
        print("FAIL: invalid or damaged FILETST1/FILESPD1 header.\r\n");
        goto done;
    }
    if (known && (size != expected_size || seed != expected_seed
                  || pass != expected_pass || speed_format != (known == 2))) {
        print("FAIL: header does not match the current write pass.\r\n");
        goto done;
    }
    if (speed_format) {
        unsigned int i;
        result = Fread((int)handle, (long)(SPEED_HEADER_SIZE - HEADER_SIZE), buffer);
        if (!transfer_ok("Fread header padding", result,
                         SPEED_HEADER_SIZE - HEADER_SIZE, HEADER_SIZE)) goto done;
        for (i = 0; i < SPEED_HEADER_SIZE - HEADER_SIZE; ++i) {
            if (buffer[i] != 0) {
                print("FAIL: damaged speed header padding at file offset %u.\r\n",
                      HEADER_SIZE + i);
                goto done;
            }
        }
    }
    print("Pass %lu VERIFY%s: %lu payload bytes, seed 0x%08lX\r\n",
           (unsigned long)pass, speed_format ? " (untimed)" : "",
           (unsigned long)size, (unsigned long)seed);
    pattern_init(&p, seed);
    /* Different boundaries from the writer, so both paths are exercised. */
    step = (unsigned int)((pass + 3) % 8);
    while (offset < size) {
        unsigned int i, count = chunks[step++ % 8];
        if ((uint32_t)count > size - offset) count = (unsigned int)(size - offset);
        if (cancelled()) goto done;
        result = Fread((int)handle, (long)count, buffer);
        if (!transfer_ok("Fread", result, count, header_size + offset)) goto done;
        for (i = 0; i < count; ++i) {
            unsigned char expected;
            uint32_t position = offset + i;
            unsigned int in_block = (unsigned int)(position % BUFFER_SIZE);
            if (speed_format && in_block == 0) pattern_init(&p, seed);
            expected = pattern_byte(&p);
            if (speed_format && in_block < 4)
                expected ^= (unsigned char)((position / BUFFER_SIZE)
                                            >> (24 - 8 * in_block));
            if (buffer[i] != expected) {
                print("FAIL: pass %lu, file offset %lu, expected %02X, actual %02X.\r\n",
                       (unsigned long)pass, (unsigned long)(header_size + offset + i),
                       (unsigned int)expected, (unsigned int)buffer[i]);
                goto done;
            }
        }
        offset += count;
        if (offset >= next_progress) {
            print("  Verified %lu bytes\r\n", (unsigned long)offset);
            next_progress += UINT32_C(1048576);
        }
    }
    result = Fread((int)handle, 1L, buffer);
    if (!transfer_ok("Fread EOF", result, 0, header_size + size)) goto done;
    ok = 1;
done:
    if (!close_file((int)handle)) ok = 0;
    if (ok) {
        if (!speed_format) show_timing("VERIFY", size, started);
        print("PASS: header, every payload byte and file length verified.\r\n");
    }
    return ok;
}

/* Aligned sequential file I/O. Prepare a single buffer before timing;
 * change only its first four bytes to tag each block with its index.
 * A distinct file format lets standalone verify regenerate the pattern.
 */
static int speed_pass(const char *path, uint32_t size, uint32_t seed,
                      uint32_t pass)
{
    unsigned char header[SPEED_HEADER_SIZE];
    struct pattern p;
    uint32_t offset, started, write_ticks, read_ticks, first_word;
    unsigned int i;
    long handle, result;
    int ok = 0;

    memset(header, 0, sizeof(header));
    make_header(header, size, seed, pass);
    memcpy(header, speed_magic, sizeof(speed_magic));
    put32(header + 24, SPEED_HEADER_SIZE);
    put32(header + 28, header_hash(header));
    pattern_init(&p, seed);
    for (i = 0; i < BUFFER_SIZE; ++i) buffer[i] = pattern_byte(&p);
    first_word = get32(buffer);

    print("Pass %lu SPEED WRITE: %lu bytes, 16 KiB blocks; please wait.\r\n",
          (unsigned long)pass, (unsigned long)size);
    handle = Fcreate(path, 0);
    if (handle < 0) {
        print("FAIL: Fcreate returned %ld.\r\n", handle);
        return 0;
    }
    result = Fwrite((int)handle, (long)SPEED_HEADER_SIZE, header);
    if (!transfer_ok("Fwrite speed header", result, SPEED_HEADER_SIZE, 0)) goto write_done;
    started = (uint32_t)clock();
    for (offset = 0; offset < size; offset += BUFFER_SIZE) {
        if (cancelled()) goto write_done;
        put32(buffer, first_word ^ (offset / BUFFER_SIZE));
        result = Fwrite((int)handle, (long)BUFFER_SIZE, buffer);
        if (!transfer_ok("Fwrite", result, BUFFER_SIZE,
                         SPEED_HEADER_SIZE + offset)) goto write_done;
    }
    ok = 1;
write_done:
    if (!close_file((int)handle)) ok = 0;
    if (!ok) return 0;
    write_ticks = (uint32_t)clock() - started;

    print("Pass %lu SPEED READ: %lu bytes, 16 KiB blocks; please wait.\r\n",
          (unsigned long)pass, (unsigned long)size);
    handle = Fopen(path, 0);
    if (handle < 0) {
        print("FAIL: Fopen for speed read returned %ld.\r\n", handle);
        return 0;
    }
    ok = 0;
    result = Fread((int)handle, (long)SPEED_HEADER_SIZE, buffer);
    if (!transfer_ok("Fread speed header", result, SPEED_HEADER_SIZE, 0)) goto read_done;
    if (memcmp(buffer, header, sizeof(header))) {
        print("FAIL: reopened speed header differs from the current pass.\r\n");
        goto read_done;
    }
    started = (uint32_t)clock();
    for (offset = 0; offset < size; offset += BUFFER_SIZE) {
        if (cancelled()) goto read_done;
        result = Fread((int)handle, (long)BUFFER_SIZE, buffer);
        if (!transfer_ok("Fread", result, BUFFER_SIZE,
                         SPEED_HEADER_SIZE + offset)) goto read_done;
    }
    result = Fread((int)handle, 1L, buffer);
    if (!transfer_ok("Fread EOF", result, 0, SPEED_HEADER_SIZE + size)) goto read_done;
    ok = 1;
read_done:
    if (!close_file((int)handle)) ok = 0;
    if (!ok) return 0;
    read_ticks = (uint32_t)clock() - started;

    print("Checking file contents outside the timed transfers...\r\n");
    if (!verify_file(path, 2, size, seed, pass)) return 0;
    print_timing("SPEED WRITE", size, write_ticks, "sequential file I/O");
    print_timing("SPEED READ", size, read_ticks, "sequential file I/O");
    return 1;
}

static int number(const char *s, uint32_t *out)
{
    uint32_t v = 0;
    unsigned int base = 10, digit;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
    if (!*s) return 0;
    while (*s) {
        if (*s >= '0' && *s <= '9') digit = (unsigned int)(*s - '0');
        else if (*s >= 'a' && *s <= 'f') digit = (unsigned int)(*s - 'a') + 10;
        else if (*s >= 'A' && *s <= 'F') digit = (unsigned int)(*s - 'A') + 10;
        else return 0;
        if (digit >= base || v > (UINT32_MAX - digit) / base) return 0;
        v = v * base + digit;
        ++s;
    }
    *out = v;
    return 1;
}

static void usage(void)
{
    print("FILETEST - generic TOS/EmuTOS file integrity test\r\n"
           "Usage:\r\n"
           "  FILETEST.TTP run FILE [MiB [PASSES [SEED]]]\r\n"
           "  FILETEST.TTP speed FILE [MiB [PASSES [SEED]]]\r\n"
           "  FILETEST.TTP write FILE [MiB [SEED]]\r\n"
           "  FILETEST.TTP verify FILE\r\n"
           "Defaults: 4 MiB, 10 run / 3 speed passes, seed 0x12345678.\r\n"
           "MiB: 1..64; passes: 1..10000; seed: decimal or 0x hex.\r\n"
           "Run/speed/write require a new filename. Last file is retained.\r\n"
           "ESC aborts between transfers. Verify uses the saved file header.\r\n");
}

int main(int argc, char **argv)
{
    uint32_t mib = 4, passes = 10, seed = DEFAULT_SEED, pass;
    int run, write, speed;
    if (argc < 3) { usage(); return 1; }
    speed = strcmp(argv[1], "speed") == 0;
    run = strcmp(argv[1], "run") == 0 || speed;
    if (speed) passes = 3;
    write = strcmp(argv[1], "write") == 0;
    if (strcmp(argv[1], "verify") == 0) {
        if (argc != 3) { usage(); return 1; }
        return verify_file(argv[2], 0, 0, 0, 0) ? 0 : 1;
    }
    if ((!run && !write) || argc > (run ? 6 : 5)
        || (argc > 3 && (!number(argv[3], &mib) || mib < 1 || mib > MAX_MIB))
        || (run && argc > 4 && (!number(argv[4], &passes) || passes < 1 || passes > 10000))
        || (run && argc > 5 && !number(argv[5], &seed))
        || (write && argc > 4 && !number(argv[4], &seed))) {
        usage(); return 1;
    }
    if (!new_path(argv[2])) return 1;
    if (write) passes = 1;
    print("Test file: %s (retained after testing)\r\n", argv[2]);
    for (pass = 1; pass <= passes; ++pass) {
        uint32_t current_seed = seed + pass - 1;
        if (speed) {
            if (!speed_pass(argv[2], mib * UINT32_C(1048576), current_seed, pass)) return 1;
        } else {
            if (!write_file(argv[2], mib * UINT32_C(1048576), current_seed, pass)) return 1;
            if (run && !verify_file(argv[2], 1, mib * UINT32_C(1048576), current_seed, pass)) return 1;
        }
    }
    if (speed) print("SUCCESS: %lu speed passes; untimed verification passed.\r\n", (unsigned long)passes);
    else if (run) print("SUCCESS: %lu write/close/reopen/verify passes.\r\n", (unsigned long)passes);
    else print("WRITE COMPLETE: file closed. Run verify after reboot if desired.\r\n");
    return 0;
}
