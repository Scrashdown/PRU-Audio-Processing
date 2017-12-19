CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lprussdrv

PRU_CC = pasm

all: pru1 loading

clean:
	-@rm gen/*

# Assemble pru files and move them to the gen/ directory
pru1: pru/pru1.asm
	@tput bold
	@echo "\n----- Building PRU1 (CIC) Firmware -----"
	@tput sgr0
	$(PRU_CC) -b -V3 pru/pru1.asm gen/pru1.bin

# Build the loader program
loading: host/loader.c
	@tput bold
	@echo "\n----- Building C Host Loader -----"
	@tput sgr0
	$(CC) $(CFLAGS) -o gen/loader host/loader.c $(LDFLAGS)
