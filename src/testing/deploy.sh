#! /bin/bash
# Stop on first error
set -e

## Debug LED from PRU1
echo "Debug LED out (PRU1)"
config-pin -a P8.45 pruout
config-pin -q P8.45

# Build the program
mkdir -p output
mkdir -p gen
make clean
make

# Start the program
cd gen
./loader pru1.bin
