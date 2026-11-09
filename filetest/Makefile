NAME := filetest
PREFIX ?= m68k-atari-mint
CC := $(PREFIX)-gcc
LIBCMINI ?= /usr/libcmini/lib
CPPFLAGS += -I$(LIBCMINI)/../include
CPUFLAGS ?= -m68000
CFLAGS ?= -O2 -Wall -Wextra -std=c99
SERIAL_PORT ?= /dev/ttyACM0
SERIAL_BAUD ?= 57600
# Binary transfer with control-character escaping for older receivers.
SZFLAGS ?= -b -e

.PHONY: all build clean check send send-setting-port
all: build
build: $(NAME).ttp

$(NAME).ttp: $(NAME).c Makefile
	$(CC) $(CPPFLAGS) $(CPUFLAGS) $(CFLAGS) -nostdlib $(LIBCMINI)/crt0.o $< -o $@ -s -L$(LIBCMINI) -lcmini -lgcc

check:
	python3 tests/test_host.py

clean:
	rm -f $(NAME).ttp

send: build
	@echo "Start a ZMODEM receiver on the target first; remove any old FILETEST.TTP."
	sz $(SZFLAGS) $(NAME).ttp > "$(SERIAL_PORT)" < "$(SERIAL_PORT)"

send-setting-port: build
	stty -F "$(SERIAL_PORT)" raw -echo $(SERIAL_BAUD) cs8 -parenb -cstopb clocal cread -crtscts -ixon -ixoff
	$(MAKE) send
