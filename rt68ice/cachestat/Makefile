PREFIX ?= m68k-atari-mint
CC := $(PREFIX)-gcc
LIBCMINI ?= /usr/libcmini/lib
CPPFLAGS += -I$(LIBCMINI)/../include
CFLAGS ?= -O2 -Wall -Wextra -std=c99
CPUFLAGS ?= -m68000
SERIAL_PORT ?= /dev/ttyACM0
SERIAL_BAUD ?= 57600
SZFLAGS ?= -b -e
DEPLOY_PATH ?= /home/michele/hatari/Harddisk/APPS

.PHONY: all build check clean deploy send send-setting-port
all: build
build: CACHSTAT.TOS

CACHSTAT.TOS: cachestat.c probe.S Makefile
	$(CC) $(CPPFLAGS) $(CPUFLAGS) $(CFLAGS) -nostdlib $(LIBCMINI)/crt0.o cachestat.c probe.S -o $@ -s -L$(LIBCMINI) -lcmini -lgcc

check:
	sh tests/check.sh

clean:
	rm -f CACHSTAT.TOS

deploy: build
	cp CACHSTAT.TOS "$(DEPLOY_PATH)/"

send: build
	@echo "Start a ZMODEM receiver on the target first; remove any old CACHSTAT.TOS."
	sz $(SZFLAGS) CACHSTAT.TOS > "$(SERIAL_PORT)" < "$(SERIAL_PORT)"

send-setting-port: build
	stty -F "$(SERIAL_PORT)" raw -echo $(SERIAL_BAUD) cs8 -parenb -cstopb clocal cread -crtscts -ixon -ixoff
	$(MAKE) send
