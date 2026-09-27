NAME=g_hello
TYPE=prg
DEPLOY_PATH=/home/michele/hatari/Harddisk/APPS

SERIAL_PORT = /dev/ttyACM0
SERIAL_BAUD = 57600

PREFIX=m68k-atari-mint

CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OBJCOPY=$(PREFIX)-objcopy
OBJDUMP=$(PREFIX)-objdump

#  The -Wl,--traditional-format option above is only useful when you want to use a classic Atari debugger like MonST2
#CFLAGS=-O2 -Wl,--traditional-format
#CFLAGS=-lgem

build: $(NAME).c
	m68k-atari-mint-gcc -nostdlib $(LIBCMINI)/crt0.o $(NAME).c -o $(NAME).$(TYPE) -s -L/usr/libcmini/lib -lcmini -lgcc -lgem

clean:
	rm -f $(NAME).prg

deploy:
	cp $(NAME).$(TYPE) $(DEPLOY_PATH)

# Send the binary file to EmuTOS, serial port must already be configured, e.g. with GTKTerm.
send: build
	@echo "\033[0;32mStart a zmodem client on the RT68, e.g.: XYZ.TTP\033[0m"
	@echo "\033[0;32m- serial port must already be set, e.g. opening GTKTerm))\033[0m"
	@echo "\033[0;31m- delete the old file before transferring a new one!))\033[0m"
	sz $(NAME).$(TYPE) > $(SERIAL_PORT) < $(SERIAL_PORT)

# Send the binary file to EmuTOS, configuring the serial port
# cs8 		: 8 data bits
# -parenb 	: disable parity
# cstopb	: 1 stop bit
send-setting-port: build
	@echo "\033[0;32mStart a zmodem client on the RT68, e.g.: XYZ.TTP\033[0m"
	@echo "\033[0;31m- delete the old file before transferring a new one!))\033[0m"
	stty -F $(SERIAL_PORT) $(SERIAL_BAUD) cs8 -parenb cstopb
	sz $(NAME).$(TYPE) > $(SERIAL_PORT) < $(SERIAL_PORT)
	
