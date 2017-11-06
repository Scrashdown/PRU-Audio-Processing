#! /bin/bash
# Stop on first error
set -e

# TODO: configure the pins
## CLK output pin from PRU0
config-pin -a P8.11 pruout
config-pin -q P8.11
## CLK input pin to PRU1
config-pin -a P8.45 pruin
config-pin -q P8.45
## DATA input pin to PRU1
config-pin -a P8.46 pruin
config-pin -q P8.46

# Build the program
make clean
make

# Start the program
cd gen
./loader clk_pru0.bin cic_pru1.bin
