#! /bin/bash
# Abort on first error
set -e

PWM_PERIOD_NS='800'
PWM_DUTY_CYCLE='400'

cd /sys/class/pwm/pwmchip3/
echo 0 > export
# Setting up parameters for PWM and enable it
echo PWM_PERIOD_NS > period
echo PWM_DUTY_CYCLE > duty_cycle
echo 1 > enable