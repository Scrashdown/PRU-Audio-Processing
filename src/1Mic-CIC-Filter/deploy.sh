#! /bin/bash
# Stop on first error
set -e

PWM_PERIOD_NS = 800
PWM_DUTY_CYCLE = 400

## Setup pin P9.14 for PWM (CLK) output
echo "PWM/CLK output (BeagleBone)"
config-pin -a P9.14 PWM
config-pin -q P9.14
sudo su
    cd /sys/class/pwm/pwmchip3/
    echo 0 > export
    # Setting up parameters for PWM and enable it
    echo $(PWM_PERIOD_NS) > period
    echo $(PWM_DUTY_CYCLE) > duty_cycle
    echo 1 > enable
exit

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
