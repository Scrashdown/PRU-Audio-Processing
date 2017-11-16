#! /bin/bash
# Abort on first error
set -e

echo "Setting up BeagleBone Black PWM (CLK)"

PWM_PERIOD_NS='800'
PWM_DUTY_CYCLE='400'

cd /sys/class/pwm/pwmchip3/
echo "1"
echo 0 > export
# Setting up parameters for PWM and enable it
echo "2"
echo $PWM_PERIOD_NS > period
echo "3"
echo $PWM_DUTY_CYCLE > duty_cycle
echo "4"
echo 1 > enable
echo "finish"