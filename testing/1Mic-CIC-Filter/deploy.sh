#! /bin/bash
# Stop on first error
set -e

## CLK output pin from PRU0
config-pin -a P8.11 pruout
config-pin -q P8.11
## DATA input pin to PRU1
config-pin -a P8.28 pruin
config-pin -q P8.28
## CLK input pin to PRU1
config-pin -a P8.30 pruin
config-pin -q P8.30

# Build the program
make clean
make

# Start the program
cd gen
./loader clk_pru0.bin cic_pru1.bin
