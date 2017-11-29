#! /bin/bash
# Stop on first error
set -e

## Debug LED from PRU0
echo "Debug LED out (PRU0)"
config-pin -a P8.11 pruout
config-pin -q P8.11

# Build the program
mkdir -p output
mkdir -p gen
make clean
make

# Start the program
cd gen
./loader pru1.bin
