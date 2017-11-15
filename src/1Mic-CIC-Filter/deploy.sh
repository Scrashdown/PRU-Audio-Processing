#! /bin/bash
# Stop on first error
set -e

## CLK output pin from PRU0
echo "CLK out (PRU0)"
config-pin -a P8.11 pruout
config-pin -q P8.11
## DATA input pin to PRU1
echo "DATA in (PRU1)"
config-pin -a P8.28 pruin
config-pin -q P8.28
## CLK input pin to PRU1
echo "CLK in (PRU1)"
config-pin -a P8.30 pruin
config-pin -q P8.30
## Debug LED from PRU1
echo "Debig LED out (PRU1)"
config-pin -a P8.45 pruout
config-pin -q P8.45

# Build the program
mkdir -p output
mkdir -p gen
make clean
make

# Start the program
cd gen
./loader clk_pru0.bin cic_pru1.bin
