NAME=hello
DEPLOY_PATH=/home/michele/hatari/Harddisk/APPS

#SERIAL_PORT=/dev/ttyUSB1
SERIAL_PORT = /dev/ttyACM0
#BAUD_RATE=38400
#BAUD_RATE=76800
SERIAL_BAUD = 57600

PREFIX=m68k-atari-mint

CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld

#  The -Wl,--traditional-format option above is only useful when you want to use a classic Atari debugger like MonST2
# CFLAGS=-O2 -Wl,--traditional-format
# $(CC) $(NAME).c -o $(NAME).tos $(CFLAGS) 

#CFLAGS=-O2

# The following is supposed to use a smaller libc but it makes the program fail with
# Panic: Illegal Instruction
CFLAGS=-nostdlib /usr/libcmini/lib/crt0.o -s -L/usr/libcmini/lib -lcmini -lgcc

# TODO:
# Note the order of the parameters is important as it is now using CFLAGS doesn't work
# `-nostdlib /usr/libcmini/lib/crt0.o` must be at the beginning and 
# `-s -L/usr/libcmini/lib -lcmini -lgcc` at the end

build: $(NAME).c
	m68k-atari-mint-gcc -nostdlib $(LIBCMINI)/crt0.o $(NAME).c -o $(NAME).tos -s -L/usr/libcmini/lib -lcmini -lgcc

clean:
	rm -f $(NAME).tos

deploy: build
	cp $(NAME).tos $(DEPLOY_PATH)

# Send the binary file to EmuTOS, serial port must already be configured, e.g. with GTKTerm.
send: build
	@echo "\033[0;32mStart a zmodem client on the RT68, e.g.: XYZ.TTP\033[0m"
	@echo "\033[0;32m- serial port must already be set, e.g. opening GTKTerm))\033[0m"
	@echo "\033[0;31m- delete the old file before transferring a new one!))\033[0m"
	sz $(NAME).tos > $(SERIAL_PORT) < $(SERIAL_PORT)	

# Send the binary file to EmuTOS, configuring the serial port
# cs8 		: 8 data bits
# -parenb 	: disable parity
# cstopb	: 1 stop bit
send-setting-port: build
	@echo "\033[0;32mStart a zmodem client on the RT68, e.g.: XYZ.TTP\033[0m"
	@echo "\033[0;31m- delete the old file before transferring a new one!))\033[0m"
	stty -F $(SERIAL_PORT) $(BAUD_RATE) cs8 -parenb cstopb
	sz $(NAME).tos > $(SERIAL_PORT) < $(SERIAL_PORT)	
