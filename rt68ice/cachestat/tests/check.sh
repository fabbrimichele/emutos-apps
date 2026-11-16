#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT HUP INT TERM
cc -std=c99 -Wall -Wextra -Werror -O2 -DCACHESTAT_HOST_TEST -Itests cachestat.c tests/host.c -o "$TEST_DIR/cachestat"
EXPECT_COMMAND=4 "$TEST_DIR/cachestat" STOP > "$TEST_DIR/out"
grep -q 'Read hit rate.*75.00%' "$TEST_DIR/out"
grep -q 'Elapsed seconds.*2.000' "$TEST_DIR/out"
grep -q 'Read stall / read.*2.500' "$TEST_DIR/out"
grep -q 'RAM wait / read miss.*6.000' "$TEST_DIR/out"
grep -q 'Counting: stopped; snapshot: valid' "$TEST_DIR/out"
RUNNING=1 EXPECT_COMMAND=4 "$TEST_DIR/cachestat" show > "$TEST_DIR/out"
grep -q 'Counting: stopped' "$TEST_DIR/out"
EXPECT_COMMAND=3 "$TEST_DIR/cachestat" start > "$TEST_DIR/out"
EXPECT_COMMAND=2 "$TEST_DIR/cachestat" clear > "$TEST_DIR/out"
EXPECT_COMMAND=1 "$TEST_DIR/cachestat" resume > "$TEST_DIR/out"
ZERO=1 "$TEST_DIR/cachestat" > "$TEST_DIR/out"
grep -q 'Read hit rate.*n/a' "$TEST_DIR/out"
LARGE=1 "$TEST_DIR/cachestat" > "$TEST_DIR/out"
grep -q '1311768469162688512' "$TEST_DIR/out"
if MISSING=1 "$TEST_DIR/cachestat" STOP > "$TEST_DIR/out"; then exit 1; fi
grep -q 'hardware not found' "$TEST_DIR/out"
if BAD_VERSION=1 "$TEST_DIR/cachestat" > "$TEST_DIR/out"; then exit 1; fi
grep -q 'Unsupported' "$TEST_DIR/out"
if "$TEST_DIR/cachestat" wrong > "$TEST_DIR/out"; then exit 1; fi
if "$TEST_DIR/cachestat" start extra > "$TEST_DIR/out"; then exit 1; fi
"$TEST_DIR/cachestat" HELP > "$TEST_DIR/out"
echo 'CACHSTAT host checks passed.'
