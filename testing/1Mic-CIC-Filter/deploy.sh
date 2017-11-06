#! /bin/bash
# Stop on first error
set -e

# TODO: configure the pins

# Build the program
make clean
make

# Start the program
cd gen
./loader clk_pru0.bin cic_pru1.bin
