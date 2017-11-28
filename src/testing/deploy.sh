#! /bin/bash
# Stop on first error
set -e

# Build the program
mkdir -p output
mkdir -p gen
make clean
make

# Start the program
cd gen
./loader pru0.bin
