CC = gcc
CFLAGS = -Wall
LDFLAGS = -lprussdrv -lpthread

PRU_CC = pasm

all: pru1 loading

clean:
	-@rm gen/*

# Assemble pru files and move them to the gen/ directory
pru1: pru/pru1.asm
	@tput bold
	@echo "\n----- Building PRU1 (CIC) Firmware -----"
	@tput sgr0
	$(PRU_CC) -b -V3 pru/pru1.asm
	@mv pru1.bin gen/

HOST_FILES = $(addprefix host/, main.c loader.c loader.h interface.c interface.h ringbuffer.c ringbuffer.h)

# Build the loader program
loading: $(HOST_FILES)
	@tput bold
	@echo "\n----- Building C Host Loader -----"
	@tput sgr0
	$(CC) $(CFLAGS) -o main $(HOST_FILES) $(LDFLAGS)
	@mv main gen/
