#! /bin/bash
# Abort on first error
set -e

echo "Setting up BeagleBone Black PWM (CLK)"

PWM_PERIOD_NS='800'
PWM_DUTY_CYCLE='400'

cd /sys/class/pwm/pwmchip3/
# Check pwm0 does not exist, if it exists, the next command will cause an I/O error
if [ ! -d 'pwm0' ]; then
    # This will create
    echo 0 > export
fi

# Setting up parameters for PWM and enable it
cd pwm0/
echo $PWM_PERIOD_NS > period
echo $PWM_DUTY_CYCLE > duty_cycle
echo 1 > enable