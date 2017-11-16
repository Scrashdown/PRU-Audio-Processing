#! /bin/bash
# Stop on first error
set -e

## Setup pin P9.14 for PWM (CLK) output
echo "PWM/CLK output (BeagleBone)"
config-pin -a P9.14 pwm
config-pin -q P9.14
sudo sh "PWMsetup.sh"

## DATA input pin to PRU1
echo "DATA in (PRU1)"
config-pin -a P8.28 pruin
config-pin -q P8.28
## CLK input pin to PRU1
echo "CLK in (PRU1)"
config-pin -a P8.30 pruin
config-pin -q P8.30
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
./loader cic_pru1.bin
