/* Host-only deterministic TOS tick clock. SPDX-License-Identifier: MIT */
#ifndef FILETEST_TEST_TIME_H
#define FILETEST_TEST_TIME_H
#define CLOCKS_PER_SEC 200
typedef long clock_t;
clock_t clock(void);
#endif
