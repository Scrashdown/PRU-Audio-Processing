#! /bin/bash
# Stop on first error
set -e

## Setup PWM for CLK signal
echo "PWM/CLK output (BeagleBone)"
config-pin -a P9.14 pwm
config-pin -q P9.14

PWM_PERIOD_NS='950'
PWM_DUTY_CYCLE='475'
sudo sh "../utils/PWMsetup.sh $PWM_PERIOD_NS $PWM_DUTY_CYCLE"

## DATA input pins to PRU1
echo "DATA1 in (PRU1)"
config-pin -a P8.28 pruin
config-pin -q P8.28
echo "DATA2 in (PRU1)"
config-pin -a P8.27 pruin
config-pin -q P8.27
echo "DATA3 in (PRU1)"
config-pin -a P8.29 pruin
config-pin -q P8.29
echo "DATA4 in (PRU1)"
config-pin -a P8.40 pruin
config-pin -q P8.40
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
./loader pru1.bin
