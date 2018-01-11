# How to use the built-in PWM on the BeagleBone Black

*Note: this was tested on this image:* `Linux beaglebone 4.4.91-ti-r133 #1 SMP Tue Oct 10 05:18:08 UTC 2017 armv7l GNU/Linux`

Based on this very useful link : https://www.teachmemicro.com/beaglebone-black-pwm-ubuntu-device-tree/

By default on this image, it shouldn't be required to load a special device tree to use the built-in PWM pin. We will use pin P9.14 in this example.

First, enable pwm mode on pin P9.14 by running this command :

    $ config-pin -a P9.14 pwm

Run the following command to check everything worked properly :

    $ config-pin -q P9.14
    P9_14 Mode: pwm

We will now have to set the parameters of the pwm. To do so, run the following commands :

    $ sudo su
    # cd /sys/class/pwm
    # cd pwmchip3
    # echo 0 > export

The last command should make the folder `pwm0` appear. `cd pwm0/` and you should find the following files :

    # ls -al
    total 0
    drwxr-xr-x 3 root root    0 Nov 15 15:59 .
    drwxrwxr-x 4 root pwm     0 Nov 15 15:59 ..
    -rw-r--r-- 1 root root 4096 Nov 15 16:12 duty_cycle
    -rw-r--r-- 1 root root 4096 Nov 15 16:11 enable
    -rw-r--r-- 1 root root 4096 Nov 15 16:12 period
    -rw-r--r-- 1 root root 4096 Nov 15 16:05 polarity
    drwxr-xr-x 2 root root    0 Nov 15 16:26 power
    -rw-r--r-- 1 root root 4096 Nov 15 16:26 uevent

These files allow you to modify the PWM parameters by echoing values to them. For example, to get a 1.235MHz signal with a 50% duty cycle, use the following parameters :

    # echo 800 > period
    # echo 400 > duty_cycle

*Note: the value assigned to duty_cycle must always be smaller or equal to the value assigned to period.*

Finally, to enable the PWM :

    # echo 1 > enable

Likewise, if you want to disable it :

    # echo 0 > enable