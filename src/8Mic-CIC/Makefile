CC = gcc
CFLAGS = -Wall
LDFLAGS = -lprussdrv

PRU_CC = pasm

all: pru0 loading

clean:
	-@rm gen/*

# Assemble pru files and move them to the gen/ directory
pru0: pru0.asm prudefs.hasm
	@tput bold
	@echo "\n----- Building PRU0 Firmware -----"
	@tput sgr0
	$(PRU_CC) -b -V3 pru0.asm
	mv *.bin gen/

# Build the loader program
loading: loader.c
	@tput bold
	@echo "\n----- Building C Host Loader -----"
	@tput sgr0
	$(CC) $(CFLAGS) -o loader loader.c $(LDFLAGS)
	mv loader gen/
